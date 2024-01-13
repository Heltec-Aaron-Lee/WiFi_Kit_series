#include "HT_st7735.h"
#include "Arduino.h"
HT_st7735 st7735;
void setup()
{  
    Serial.begin(115200);
    st7735.st7735_init();
    Serial.printf("Ready!\r\n");
}

void loop() {
    // Check border
    st7735.st7735_fill_screen(ST7735_BLACK);

    for(int x = 0; x < ST7735_WIDTH; x++) {
        st7735.st7735_draw_pixel(x, 0, ST7735_RED);
        st7735.st7735_draw_pixel(x, ST7735_HEIGHT-1, ST7735_RED);
    }

    for(int y = 0; y < ST7735_HEIGHT; y++) {
        st7735.st7735_draw_pixel(0, y, ST7735_RED);
        st7735.st7735_draw_pixel(ST7735_WIDTH-1, y, ST7735_RED);
    }

    delay(3000);

    // Check fonts
    st7735.st7735_fill_screen(ST7735_BLACK);
    st7735.st7735_write_str(0, 0, "Font_7x10, red on black, lorem ipsum dolor sit amet", Font_7x10, ST7735_RED, ST7735_BLACK);
    st7735.st7735_write_str(0, 3*10, "Font_11x18, green, lorem ipsum", Font_11x18, ST7735_GREEN, ST7735_BLACK);
    st7735.st7735_write_str(0, 3*10+3*18, "Font_16x26", Font_16x26, ST7735_BLUE, ST7735_BLACK);
    delay(2000);

    // Check colors
    st7735.st7735_fill_screen(ST7735_BLACK);
    st7735.st7735_write_str(0, 0, "BLACK", Font_11x18, ST7735_WHITE, ST7735_BLACK);
    delay(500);

    st7735.st7735_fill_screen(ST7735_BLUE);
    st7735.st7735_write_str(0, 0, "BLUE", Font_11x18, ST7735_BLACK, ST7735_BLUE);
    delay(500);
 
    st7735.st7735_fill_screen(ST7735_RED);
    st7735.st7735_write_str(0, 0, "RED", Font_11x18, ST7735_BLACK, ST7735_RED);
    delay(500);

    st7735.st7735_fill_screen(ST7735_GREEN);
    st7735.st7735_write_str(0, 0, "GREEN", Font_11x18, ST7735_BLACK, ST7735_GREEN);
    delay(500);

    st7735.st7735_fill_screen(ST7735_CYAN);
    st7735.st7735_write_str(0, 0, "CYAN", Font_11x18, ST7735_BLACK, ST7735_CYAN);
    delay(500);

    st7735.st7735_fill_screen(ST7735_MAGENTA);
    st7735.st7735_write_str(0, 0, "MAGENTA", Font_11x18, ST7735_BLACK, ST7735_MAGENTA);
    delay(500);

    st7735.st7735_fill_screen(ST7735_YELLOW);
    st7735.st7735_write_str(0, 0, "YELLOW", Font_11x18, ST7735_BLACK, ST7735_YELLOW);
    delay(500);

    st7735.st7735_fill_screen(ST7735_WHITE);
    st7735.st7735_write_str(0, 0, "WHITE", Font_11x18, ST7735_BLACK, ST7735_WHITE);
    delay(500);
}