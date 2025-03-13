// Define delays for different types of operations (in microseconds)
static const uint8_t chardelay = 250;   // Delay for sending simple characters
static const uint8_t longdelay = 4000;  // Delay for long instructions (e.g., screen clear, beep)
static const uint8_t cmdbytedelay = 250;
static const uint8_t pixdelay = 400;    // Delay for sending pixel data

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
  send_char(0x90, chardelay); // Set background to black

  // Draw characters in different foreground colors (0x81-0x8F)
  for(uint8_t i = 0x81; i <= 0x8F; i++) {
    send_char(i, chardelay);
    for(char val = ' '; val <= '~'; val++) { // ASCII printable range
      send_char(val, chardelay);
    }
  }

  handle_serial();

  send_char(0x80, chardelay); // Set foreground to black
  for(uint8_t i = 0x91; i <= 0x9F; i++) {
    send_char(i, chardelay);
    for(char val = ' '; val <= '~'; val++) {
      send_char(val, chardelay);
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

// Function to send a 32-bit pixel instruction over parallel port
void send_pixelword(uint32_t instruction) {
  send_char(instruction, cmdbytedelay);         // Send least significant byte
  send_char((instruction >> 8), cmdbytedelay);  // Send next byte
  send_char((instruction >> 16), cmdbytedelay); // Send next byte
  send_char((instruction >> 24), pixdelay);     // Send most significant byte
}

// Function to draw a filled square at (x,y) with a given size and color
void draw_square(uint16_t x, uint16_t y, uint16_t size, uint8_t color) {
    // Ensure the square stays within bounds
    if (x + size > SCREEN_WIDTH) size = SCREEN_WIDTH - x;
    if (y + size > SCREEN_HEIGHT) size = SCREEN_HEIGHT - y;

    // Loop through each pixel in the square
    for (uint16_t i = 0; i < size; i++) {
        for (uint16_t j = 0; j < size; j++) {
            uint32_t instruction = generate_pixel_command(x + i, y + j, color);
            send_pixelword(instruction);
            handle_serial(); // Process serial communication during drawing
        }
    }
}

// Function to draw a horizontal line at (x,y) with a given length and color
void draw_line(uint16_t x, uint16_t y, uint16_t size, uint8_t color) {
    // Ensure the line stays within bounds
    if (x + size > SCREEN_WIDTH) size = SCREEN_WIDTH - x;
    if (y >= SCREEN_HEIGHT) return; // Ensure y is within bounds

    // Loop to draw the line
    for (uint16_t i = 0; i < size; i++) {
        uint32_t instruction = generate_pixel_command(x + i, y, color);
        send_pixelword(instruction);
        handle_serial(); // Process serial communication during drawing
    }
}

// Function to draw a graphical test pattern
void draw_graph() {
  send_char(0xFF, longdelay); // clear screen 
  send_char(0xA0, longdelay); // Beep
  send_char(0xB0, longdelay); // pixel mode
  
  const uint16_t sz = 5; // Square size

  // Draw corner squares
  draw_square(0, 0, sz, 0x0F);
  draw_square(0, SCREEN_HEIGHT - sz - 1, sz, 0x0F);
  draw_square(SCREEN_WIDTH - sz - 1, 0, sz, 0x0F);
  draw_square(SCREEN_WIDTH - sz - 1, SCREEN_HEIGHT - sz - 1, sz, 0x0F);

  // Draw color test bars
  for(uint8_t j = 0; j < 2; j++) {
    for(uint8_t i = 0; i < 2; i++) {
      draw_square(50 + i * 15, 50 + j * 15, 5, j*16 + i);
    }
  }

  send_pixelword(0xAA55AA55);
  _delay_ms(1);
  handle_serial();
}

// Main loop function - alternates between text and graphical tests
void loop() {
  //draw_text();   // Draw text pattern
  //_delay_ms(3000); 

  draw_graph();  // Draw graphical pattern
  _delay_ms(1000);
}