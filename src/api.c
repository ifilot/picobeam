#include "api.h"

static short posx = 0;
static short posy = 0;
static char fg_color = WHITE;
static char bg_color = BLACK;
static uint32_t pixelword;
static bool flag_pixel = false;
static uint8_t nrbytes = 0;

void parse_input(char ch) {

    static uint8_t mode = MODE_TEXT;

    if(flag_pixel) {
        pixelword |= ((uint32_t)ch << (nrbytes * 8));
        nrbytes++;

        if(nrbytes == 4) {
            if(pixelword != 0xAA55AA55) {
                draw_pixel_from_word(pixelword);
                pixelword = 0;
                nrbytes = 0;
            } else {
                flag_pixel = false;
            }
            pixelword = 0;
            nrbytes = 0;
        }
        return;
    }

    if(ch >= 0x80) {  // check for control bytes
        switch(ch) {
            case 0x80:
                fg_color = BLACK;
            break;
            case 0x81:
                fg_color = DARK_GREEN;
            break;
            case 0x82:
                fg_color = MED_GREEN;
            break;
            case 0x83:
                fg_color = GREEN;
            break;
            case 0x84:
                fg_color = DARK_BLUE;
            break;
            case 0x85:
                fg_color = BLUE;
            break;
            case 0x86:
                fg_color = LIGHT_BLUE;
            break;
            case 0x87:
                fg_color = CYAN;
            break;
            case 0x88:
                fg_color = RED;
            break;
            case 0x89:
                fg_color = DARK_ORANGE;
            break;
            case 0x8A:
                fg_color = ORANGE;
            break;
            case 0x8B:
                fg_color = YELLOW;
            break;
            case 0x8C:
                fg_color = MAGENTA;
            break;
            case 0x8D:
                fg_color = PINK;
            break;
            case 0x8E:
                fg_color = LIGHT_PINK;
            break;
            case 0x8F:
                fg_color = WHITE;
            break;
            case 0x90:
                bg_color = BLACK;
            break;
            case 0x91:
                bg_color = DARK_GREEN;
            break;
            case 0x92:
                bg_color = MED_GREEN;
            break;
            case 0x93:
                bg_color = GREEN;
            break;
            case 0x94:
                bg_color = DARK_BLUE;
            break;
            case 0x95:
                bg_color = BLUE;
            break;
            case 0x96:
                bg_color = LIGHT_BLUE;
            break;
            case 0x97:
                bg_color = CYAN;
            break;
            case 0x98:
                bg_color = RED;
            break;
            case 0x99:
                bg_color = DARK_ORANGE;
            break;
            case 0x9A:
                bg_color = ORANGE;
            break;
            case 0x9B:
                bg_color = YELLOW;
            break;
            case 0x9C:
                bg_color = MAGENTA;
            break;
            case 0x9D:
                bg_color = PINK;
            break;
            case 0x9E:
                bg_color = LIGHT_PINK;
            break;
            case 0x9F:
                bg_color = WHITE;
            break;
            case 0xA0:
                flag_beep = true;
            break;
            case 0xB0:
                flag_pixel = true;
                pixelword = 0;
                nrbytes = 0;
            break;
            case 0xFF:
                clear_screen();
                posy = 0;
                posx = 0;
                flag_beep = false;
                flag_pixel = false;
            break;
        }
    } else {
        if(posy + CHARHEIGHT >= SCREENHEIGHT) {
            clear_screen();
            posy = 0;
            posx = 0;
            flag_pixel = false;
        }
    
        if(ch == '\n') {
            posy += CHARHEIGHT;
            posx = 0;
        } else {
            draw_character(posx, posy, ch, fg_color, bg_color);
    
            posx += 8;
            if (posx >= SCREENWIDTH-1) {
                posx = 0;
                posy += CHARHEIGHT;
            }
        }
    }
}