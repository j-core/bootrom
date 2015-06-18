#ifndef LCD_FONT_H
#define LCD_FONT_H

struct lcd_char {
  unsigned char low_page[8];
  unsigned char high_page[8];
};

extern const struct lcd_char lcd_font_chars[];

#endif
