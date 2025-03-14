#include "api.h"

static uint16_t pixelword;
static uint16_t charword;
static bool flag_pixel = false;
static bool flag_char = false;
static uint8_t nrbytes = 0;

void parse_input(char ch) {

    if(flag_pixel) {
        pixelword |= ch;
        process_pixelword(pixelword);
        pixelword = 0;
        flag_pixel = false;
        return;
    }

    if(flag_char) {
        charword |= ch;
        process_charword(charword);
        charword = 0;
        flag_char = false;
        return;
    }

    if(ch >= 0x80) {  // check for control bytes

        if((ch & 0xF0) == 0xB0) {
            flag_pixel = true;
            pixelword = (uint16_t)ch << 8;     // store upper byte
            return;
        }

        if((ch & 0xF0) == 0xC0) {
            flag_char = true;
            charword = (uint16_t)ch << 8;     // store upper byte
            return;
        }

        switch(ch) {
            case 0xA0:
                flag_beep = true;
            break;
            case 0xFF:
                clear_screen();
                pposy = 0;
                pposx = 0;
                flag_beep = false;
                flag_pixel = false;
            break;
        }
    } else {
        // todo: replace with print char command
        if(pposy + CHARHEIGHT >= SCREENHEIGHT) {
            clear_screen();
            pposy = 0;
            pposx = 0;
            flag_pixel = false;
        }
    
        if(ch == '\n') {
            cposy += CHARHEIGHT;
            cposx = 0;
        } else {
            draw_character(cposx, cposy, ch, fg_color, bg_color);
    
            cposx += 8;
            if (cposx >= SCREENWIDTH-1) {
                cposx = 0;
                cposy += CHARHEIGHT;
            }
        }
    }
}