#include "vga.h"

// include font (obtained from Philips P2000C)
#include "font.c"

// Pixel color array that is DMA's to the PIO machines and a pointer to the
// ADDRESS of this color array. Note that this array is automatically
// initialized to all 0's (black)
unsigned char vga_data_array[TXCOUNT];

// pointer to first element of the vga_data_array, used for DMA
char *address_pointer = &vga_data_array[0];

// flags to control beeper
bool flag_beep = false;
bool flag_beeping = false;

/**
 * @brief Initialize the screen
 * 
 */
void init_screen() {
    // Choose which PIO instance to use (there are two instances, each with 4 state machines)
    PIO pio = pio0;

    // Our assembled program needs to be loaded into this PIO's instruction
    // memory. This SDK function will find a location (offset) in the
    // instruction memory where there is enough space for our program. We need
    // to remember these locations!
    //
    // We only have 32 instructions to spend! If the PIO programs contain more
    // than 32 instructions, then an error message will get thrown at these
    // lines of code.
    //
    // The program name comes from the .program part of the pio file and is of
    // the form <program name_program>
    uint hsync_offset = pio_add_program(pio, &hsync_program);
    uint vsync_offset = pio_add_program(pio, &vsync_program);
    uint rgb_offset =   pio_add_program(pio, &rgb_program);

    // Manually select a few state machines from pio instance pio0.
    uint hsync_sm = 0;
    uint vsync_sm = 1;
    uint rgb_sm = 2;

    // Call the initialization functions that are defined within each PIO file.
    // Why not create these programs here? By putting the initialization
    // function in the pio file, then all information about how to use/setup
    // that state machine is consolidated in one place. Here in the C, we then
    // just import and use it.
    hsync_program_init(pio, hsync_sm, hsync_offset, HSYNC_PIN);
    vsync_program_init(pio, vsync_sm, vsync_offset, VSYNC_PIN);
    rgb_program_init(pio, rgb_sm, rgb_offset, COLOR_PIN);

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // ============================== PIO DMA Channels =================================================
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    // DMA channels
    // 0 sends color data
    // 1 reconfigures and restarts 0
    // 2 latches data
    int rgb_chan_0 = dma_claim_unused_channel(true);
    int rgb_chan_1 = dma_claim_unused_channel(true);

    // Channel Zero (sends color data to PIO VGA machine)
    dma_channel_config c0 = dma_channel_get_default_config(rgb_chan_0); // default configs
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_8);             // 8-bit txfers
    channel_config_set_read_increment(&c0, true);                       // yes read incrementing
    channel_config_set_write_increment(&c0, false);                     // no write incrementing
    channel_config_set_dreq(&c0, DREQ_PIO0_TX2);                        // DREQ_PIO0_TX2 pacing (FIFO)
    channel_config_set_chain_to(&c0, rgb_chan_1);                       // chain to other channel

    dma_channel_configure(
        rgb_chan_0,        // Channel to be configured
        &c0,               // The configuration we just created
        &pio->txf[rgb_sm], // write address (RGB PIO TX FIFO)
        &vga_data_array,   // The initial read address (pixel color array)
        TXCOUNT,           // Number of transfers; in this case each is 1 byte.
        false              // Don't start immediately.
    );

    // Channel One (reconfigures the first channel)
    dma_channel_config c1 = dma_channel_get_default_config(rgb_chan_1); // default configs
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);            // 32-bit txfers
    channel_config_set_read_increment(&c1, false);                      // no read incrementing
    channel_config_set_write_increment(&c1, false);                     // no write incrementing
    channel_config_set_chain_to(&c1, rgb_chan_0);                       // chain to other channel

    dma_channel_configure(
        rgb_chan_1,                        // Channel to be configured
        &c1,                               // The configuration we just created
        &dma_hw->ch[rgb_chan_0].read_addr, // Write address (channel 0 read address)
        &address_pointer,                  // Read address (POINTER TO AN ADDRESS)
        1,                                 // Number of transfers, in this case each is 4 byte
        false                              // Don't start immediately.
    );

    ////////////////////////////////////////////////////////////////////////////
    // INITIALIZE STATE MACHINES
    ////////////////////////////////////////////////////////////////////////////

    // Initialize PIO state machine counters. This passes the information to the
    // state machines that they retrieve in the first 'pull' instructions,
    // before the .wrap_target directive in the assembly. Each uses these values
    // to initialize some counting registers.
    pio_sm_put_blocking(pio, hsync_sm, H_ACTIVE);
    pio_sm_put_blocking(pio, vsync_sm, V_ACTIVE);
    pio_sm_put_blocking(pio, rgb_sm, RGB_ACTIVE);

    // Start the two pio machine IN SYNC Note that the RGB state machine is
    // running at full speed, so synchronization doesn't matter for that one.
    // But, we'll start them all simultaneously anyway.
    pio_enable_sm_mask_in_sync(pio, ((1u << hsync_sm) | (1u << vsync_sm) | (1u << rgb_sm)));

    // Start DMA channel 0. Once started, the contents of the pixel color array
    // will be continously DMA's to the PIO machines that are driving the
    // screen. To change the contents of the screen, we need only change the
    // contents of that array.
    dma_start_channel_mask((1u << rgb_chan_0));
}

/**
 * @brief Clear the screen
 * 
 */
void clear_screen() {
    memset(vga_data_array, 0x00, TXCOUNT);
}

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
void draw_pixel(short x, short y, char color) {
    if((x > 639) | (x < 0) | (y > 479) | (y < 0) ) return;

    // compute the pixel index in the VGA data array
    //int pixel = ((640 * y) + x);

    // determine if the pixel is stored in the upper or lower 4 bits of the byte
    // if (pixel & 1) {    // odd pixel (upper 4 bits)
    //     vga_data_array[pixel >> 1] = (vga_data_array[pixel >> 1] & TOPMASK) | (color << 4);
    // }
    // else {              // even pixel (lower 4 bits)
    //     vga_data_array[pixel >> 1] = (vga_data_array[pixel >> 1] & BOTTOMMASK) | (color);
    // }
    vga_data_array[(640 * y) + x] = color;
}

/**
 * @brief Draw a character onto the screen
 * 
 * @param x         Character x-position
 * @param y         Character y-position
 * @param c         Character to print
 * @param color     Foreground color
 * @param bg        Background color
 */
void draw_character(short x, short y, unsigned char c, char color, char bg) {
    char i, j;

    // loop over rows
    for (i = 0; i < CHARHEIGHT; i++) {
        unsigned char line;

        // Fetch character pixel data from font array
        if (i == CHARHEIGHT) {// Extra column for spacing
            line = 0x00;
        } else {
            line = pgm_read_byte(font + (c * CHARHEIGHT) + i);
        }

        // Loop through each column of the character (8 columns per character)
        for (j = 0; j < 8; j++) {
            if (line & 0x1) { // If the bit is set, draw foreground color pixel
                draw_pixel(x + j, y + i, color);
            }
            else if (bg != color) { // If the bit is not set, draw background color
                draw_pixel(x + j, y + i, bg);
            }

            line >>= 1; // bitshift to the right to grab the next pixel
        }
    }
}

void draw_pixel_from_word(uint32_t pixelword) {
    // Extract X, Y, and color from pixelword (direct bit manipulation)
    uint16_t x = (pixelword >> 4) & 0x3FF;   // X position (10 bits)
    uint16_t y = (pixelword >> 14) & 0x3FF;  // Y position (10 bits)
    uint8_t color = (pixelword >> 24) & 0xFF; // Color (8 bits)

    // Bounds check (fast conditional with bitwise OR)
    if ((x >= SCREENWIDTH) | (y >= SCREENHEIGHT)) return;

    // Compute pixel index in the VGA data array
    //int pixel = (SCREENWIDTH * y) + x;
    
    // Update the VGA array (avoiding redundant shifts)
    vga_data_array[(640 * y) + x] = color;

    // if (pixel & 1) {  
    //     // Odd pixel → Upper 4 bits
    //     *vga_byte = (*vga_byte & TOPMASK) | (color << 4);
    // } else {  
    //     // Even pixel → Lower 4 bits
    //     *vga_byte = (*vga_byte & BOTTOMMASK) | color;
    // }
    
}
