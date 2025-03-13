/**
 *
 * HARDWARE CONNECTIONS
 *  - GPIO 16 ---> VGA Hsync
 *  - GPIO 17 ---> VGA Vsync
 *  - GPIO 18 ---> 470 ohm resistor ---> VGA Green 
 *  - GPIO 19 ---> 330 ohm resistor ---> VGA Green
 *  - GPIO 20 ---> 330 ohm resistor ---> VGA Blue
 *  - GPIO 21 ---> 330 ohm resistor ---> VGA Red
 *  - RP2040 GND ---> VGA GND
 *
 * RESOURCES USED
 *  - PIO state machines 0, 1, and 2 on PIO instance 0
 *  - DMA channels 0, 1, 2, and 3
 *  - 153.6 kBytes of RAM (for pixel color data)
 *
 */

#ifndef _VGA_H
#define _VGA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pico/stdlib.h>
#include <hardware/pio.h>
#include <hardware/dma.h>

#include "hsync.pio.h"
#include "vsync.pio.h"
#include "rgb.pio.h"

// Screen width/height
#define SCREENWIDTH     640     // width of the screen
#define SCREENHEIGHT    480     // height of the screen
#define CHARHEIGHT      12      // character width
#define CHARWIDTH        8      // character height

#define LATCH_PIN        2      // GPIO for latch signal
#define HSYNC_PIN       12
#define VSYNC_PIN       13
#define COLOR_PIN       14
#define DATA_BASE_PIN    4        // First data pin

// VGA timing constants
#define H_ACTIVE 655   // (active + frontporch - 1) - one cycle delay for mov
#define V_ACTIVE 479   // (active - 1)
//#define RGB_ACTIVE 319 // (horizontal active)/2 - 1
#define RGB_ACTIVE 639 // change to this if 1 pixel/byte

// Length of the pixel array, and number of DMA transfers
#define TXCOUNT (SCREENWIDTH*SCREENHEIGHT) // Total pixels/2 (since we have 2 pixels per byte)

// We can only produce 16 (4-bit) colors, so let's give them readable names -
// usable in main()
enum colors {BLACK, DARK_GREEN, MED_GREEN, GREEN,
             DARK_BLUE, BLUE, LIGHT_BLUE, CYAN,
             RED, DARK_ORANGE, ORANGE, YELLOW, 
             MAGENTA, PINK, LIGHT_PINK, WHITE} ;

// Bit masks for drawPixel routine
#define TOPMASK 0b00001111
#define BOTTOMMASK 0b11110000

// For writing text
#define tabspace 4 // number of spaces for a tab

// For accessing the font library
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))

extern bool flag_beep;
extern bool flag_beeping;

/**
 * @brief Initialize the screen
 * 
 */
void init_screen(void) ;

/**
 * @brief Clear the screen
 * 
 */
void clear_screen();

/**
 * @brief Draw a single pixel to thes creen buffer
 * 
 * @param x         Pixel x-coordinate
 * @param y         Pixel y-coordinate
 * @param color     Pixel color
 * 
 * 
 * This function modifies the contents of the VGA data array, which is
 * automatically transferred to the screen via a DMA channel.
 */
void draw_pixel(short x, short y, char color) ;

/**
 * @brief Draw a character onto the screen
 * 
 * @param x         Character x-position
 * @param y         Character y-position
 * @param c         Character to print
 * @param color     Foreground color
 * @param bg        Background color
 */
void draw_character(short x, short y, unsigned char c, char color, char bg) ;

void draw_pixel_from_word(uint32_t pixelword);

#endif // _VGA_H