// Define delays for different types of operations (in microseconds)
static const uint8_t chardelay = 250;   // Delay for sending simple characters
static const uint8_t longdelay = 4000;  // Delay for long instructions (e.g., screen clear, beep)
static const uint8_t cmdbytedelay = 250;
static const uint8_t pixdelay = 150;    // Delay for sending pixel data

// Screen dimensions
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

// Setup function - Initializes serial communication and configures output pins
void setup() {
  Serial.begin(115200);  // USB Serial (for debugging)
  Serial1.begin(115200); // Hardware Serial (for communication with external devices)

  // Set PB4-PB7 (D8-D11) and PF4-PF7 (A3-A0) as outputs
  DDRB |= 0b11110000;
  DDRF |= 0b11110000;

  // Configure latch pin (PD1) as output
  pinMode(2, OUTPUT);  
  digitalWrite(2, LOW); // Ensure latch starts low
}

// Function to send a character via parallel port with a latch pulse
void send_char(char val, unsigned int delay) {
  PORTB = (PORTB & 0x0F) | ((val & 0x0F) << 4);  // Send lower nibble to PB4-PB7
  PORTF = (PORTF & 0x0F) | ((val & 0xF0));       // Send upper nibble to PF4-PF7

  // Pulse the latch signal (D2) to indicate data is ready
  digitalWrite(2, HIGH);
  digitalWrite(2, LOW); // End latch pulse
  delayMicroseconds(delay);
}

// Function to handle serial communication between Serial (USB) and Serial1 (external device)
void handle_serial() {
  // Forward data from Serial1 (external device) to Serial (computer)
  while (Serial1.available()) {
    char incomingByte = Serial1.read();
    Serial.write(incomingByte);
  }

  // Forward data from Serial (computer) to Serial1 (external device)
  while (Serial.available()) {
    char outgoingByte = Serial.read();
    Serial1.write(outgoingByte);
  }
}

// Function to draw a full-screen text test pattern
void draw_text() {
  static uint8_t val = ' ';

  handle_serial();

  send_char(0xFF, longdelay); // Clear screen
  send_char(0xA0, longdelay); // Beep

  uint8_t col = 0;

  send_char(0xCC, chardelay);   // set background color
  send_char(0x00, chardelay);   // set background color
  for(int i=0; i<4; i++) {
    for(char val = ' '; val <= '~'; val++) {
      send_char(0xC0, chardelay);   // set foreground color
      send_char(col++, chardelay);   // set foreground color
      send_char(val, chardelay);
      if(col == 0) {
        break;
      }
    }
    if(col == 0) {
      break;
    }
  }
  send_char('\n', chardelay);

  col = 0;
  send_char(0xC0, chardelay);   // set foreground color
  send_char(0x00, chardelay);   // set foreground colo
  for(int i=0; i<4; i++) {
    for(char val = ' '; val <= '~'; val++) {
      send_char(0xCC, chardelay);       // set background color
      send_char(col++, chardelay);   // set background color
      send_char(val, chardelay);
      if(col == 0) {
        break;
      }
    }
    if(col == 0) {
      break;
    }
  }

  handle_serial();
}

// Function to generate a 32-bit pixel command
uint32_t generate_pixel_command(uint16_t x, uint16_t y, uint8_t color) {
    return ((uint32_t)(color & 0xFF) << 24) |   // 8-bit color
           ((uint32_t)(y & 0x3FF) << 14)    |   // 10-bit Y position
           ((uint32_t)(x & 0x3FF) << 4);        // 10-bit X position
}

void cmd_set_x(uint16_t x) {
    uint8_t highbyte = 0xB4 | ((x >> 8) & 3);
    uint8_t lowbyte = x & 0xFF;
    send_char(highbyte, pixdelay);
    send_char(lowbyte, pixdelay);
}

void cmd_set_y(uint16_t y) {
    uint8_t highbyte = 0xB8 | ((y >> 8) & 3);
    uint8_t lowbyte = y & 0xFF;
    send_char(highbyte, pixdelay);
    send_char(lowbyte, pixdelay);
}

void cmd_set_px(uint8_t color) {
    send_char(0xB0, pixdelay);
    send_char(color, pixdelay);
}

// Function to draw a filled square at (x,y) with a given size and color
void draw_square(uint16_t x, uint16_t y, uint16_t size, uint8_t color) {
    // Loop through each pixel in the square
    for (uint16_t i = 0; i < size; i++) {
        cmd_set_y(y+i);
        cmd_set_x(x);
        for (uint16_t j = 0; j < size; j++) {
            cmd_set_px(color);
        }
    }
}

// Function to draw a graphical test pattern
void draw_graph() {
  send_char(0xFF, longdelay); // clear screen 
  _delay_ms(1);
  //send_char(0xA0, longdelay); // Beep

  const uint16_t sz = 5; // Square size

  // Draw corner squares
  draw_square(0, 0, sz, 0xFF);
  draw_square(0, SCREEN_HEIGHT - sz - 1, sz, 0xFF);
  draw_square(SCREEN_WIDTH - sz - 1, 0, sz, 0xFF);
  draw_square(SCREEN_WIDTH - sz - 1, SCREEN_HEIGHT - sz - 1, sz, 0xFF);

  // Draw color test bars
  for(uint8_t j = 0; j < 16; j++) {
    for(uint8_t i = 0; i < 16; i++) {
      draw_square(50 + i * 15, 50 + j * 15, 5, j*16+i);
    }
  }

  _delay_ms(1);
  handle_serial();
}

// Main loop function - alternates between text and graphical tests
void loop() {
  draw_text();   // Draw text pattern
  _delay_ms(3000); 

  draw_graph();  // Draw graphical pattern
  _delay_ms(3000);
}