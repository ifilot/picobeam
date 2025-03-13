/**
 *
 * VGA Display Driver with Multicore Processing
 *
 * HARDWARE CONNECTIONS:
 *  - GPIO 16 ---> VGA Hsync
 *  - GPIO 17 ---> VGA Vsync
 *  - GPIO 18 ---> 470-ohm resistor ---> VGA Green
 *  - GPIO 19 ---> 330-ohm resistor ---> VGA Green
 *  - GPIO 20 ---> 330-ohm resistor ---> VGA Blue
 *  - GPIO 21 ---> 330-ohm resistor ---> VGA Red
 *  - RP2040 GND ---> VGA GND
 *
 * RESOURCES USED:
 *  - PIO state machines 0, 1, and 2 on PIO instance 0
 *  - DMA channels obtained via claim mechanism
 *  - 153.6 kBytes of RAM (for pixel color data)
 *
 * Protothreads v1.1.3
 * Threads:
 * Core 0:
 *  - VGA display
 *  - Blink LED25
 * Core 1:
 *  - Toggle GPIO 4
 *  - Serial I/O
 */

#include <stdio.h>
#include <stdlib.h>
#include <pico/stdlib.h>
#include <hardware/pio.h>
#include <hardware/dma.h>
#include <hardware/sync.h>
#include <hardware/timer.h>
#include <hardware/pwm.h>
#include <hardware/clocks.h>
#include <pico/multicore.h>
#include <string.h>

#include "vga.h"
#include "latch.pio.h"
#include "protothreads.h"
#include "api.h"
 
char ch;
bool flag_latch = false;

// ==================================================
// === VGA Graphics Thread (Core 0) ===
// ==================================================
static PT_THREAD(protothread_graphics(struct pt *pt)) {
    PT_BEGIN(pt);

    while (true) {
        // Draw new character if received
        if (flag_latch) {
            parse_input(ch);

            // only release when char is done
            flag_latch = false;
        }

        // Yield to allow other threads to run
        PT_YIELD_usec(100);
    }
    PT_END(pt);
}

// ==================================================
// === LED Blink Thread (Core 0) ===
// ==================================================
static PT_THREAD(protothread_toggle25(struct pt *pt)) {
    PT_BEGIN(pt);
    static bool LED_state = false;

    // Initialize onboard LED (GPIO 25)
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, true);

    PT_INTERVAL_INIT();

    while (1) {
        // Toggle LED every 100ms
        PT_YIELD_INTERVAL(100000);
        LED_state = !LED_state;
        gpio_put(25, LED_state);
    }
    PT_END(pt);
}

// ==================================================
//            Serial Input Thread (Core 1) 
// ==================================================
static PT_THREAD(protothread_serial(struct pt *pt)) {
    PT_BEGIN(pt);

    while (true) {
        // Spawn a thread to handle non-blocking serial write
        serial_write;

        // Spawn a thread to handle non-blocking serial read
        serial_read;

        PT_YIELD(pt);
    }
    PT_END(pt);
}

// ==================================================
//            Latch Input Thread (Core 1) 
// ==================================================
static PT_THREAD(protothread_latch(struct pt *pt)) {
    PT_BEGIN(pt);

    // Choose PIO and state machine
    PIO pio = pio1;
    uint sm = 0;

    // Initialize PIO program
    uint offset = pio_add_program(pio, &latch_program);
    latch_program_init(pio, sm, offset, LATCH_PIN, DATA_BASE_PIN);

    while (true) {
        if(!pio_sm_is_rx_fifo_empty(pio, sm) && !flag_latch) {
            ch = pio_sm_get(pio, sm);
            flag_latch = true;
        } else {
            PT_YIELD(pt);
        }
    }
    PT_END(pt);
}

// ==================================================
//            Latch Input Thread (Core 1) 
// ==================================================
static PT_THREAD(protothread_beeper(struct pt *pt)) {
    PT_BEGIN(pt);

    static uint64_t start_time;

    uint buzzer_pin = 3;
    uint freq = 726;
    float duty_cycle = 0.2;

    gpio_set_function(buzzer_pin, GPIO_FUNC_PWM);  // Set GPIO to PWM mode
    uint slice = pwm_gpio_to_slice_num(buzzer_pin);  // Get PWM slice
    uint channel = pwm_gpio_to_channel(buzzer_pin);  // Get PWM channel

    uint32_t clkdiv = 50; // Clock divider (adjustable for fine tuning)
    uint32_t wrap = (clock_get_hz(clk_sys) / (clkdiv * freq)) - 1;
    
    pwm_set_clkdiv(slice, clkdiv);  // Set clock divider
    pwm_set_wrap(slice, wrap);  // Set PWM frequency
    pwm_set_chan_level(slice, channel, (uint16_t)(wrap * duty_cycle));  // Set duty cycle

    while (true) {
        PT_YIELD_UNTIL(pt, flag_beep && !flag_beeping);
            flag_beep = false;
            flag_beeping = true;
            start_time = time_us_64(); 
            pwm_set_enabled(slice, true);  // Enable PWM
            printf("BEEP!\n");

        PT_YIELD_UNTIL(pt, flag_beeping && ((time_us_64() - start_time) >= (200 * 1000)));

            pwm_set_enabled(slice, false);
            flag_beeping = false;
    }
    PT_END(pt);
}

/**
 * @brief Main function for CORE 1
 * 
 */
void core1_main() {
    pt_add_thread(protothread_serial);
    pt_add_thread(protothread_latch);
    pt_add_thread(protothread_beeper);
    pt_schedule_start;
    // Never exits
}

/**
 * @brief Main function for CORE 2
 * 
 * @return int exit status
 */
int main() {
    // Initialize standard I/O
    stdio_init_all();
    printf("\nPICO2 VGA INITIALIZED\n");

    // Initialize VGA display
    init_screen();

    // Start core 1 processing
    multicore_reset_core1();
    multicore_launch_core1(&core1_main);

    // Add threads for core 0
    pt_add_thread(protothread_graphics);
    pt_add_thread(protothread_toggle25);

    // Start thread scheduling (never exits)
    pt_schedule_start;
} 