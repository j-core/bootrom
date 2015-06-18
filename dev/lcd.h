#ifndef LCD_H
#define LCD_H

/*
  Currently we support two LCD screens. This messy header file
  provides empty stub functions when they are not supported by a
  particular LCD or when no LCD is configured.

  Support of the "OLDLCD" is only partial as I don't have one to test
  with. We'll likely be phasing it out and removing it anyway.
 */

#if CONFIG_GRLCD == 1 || CONFIG_OLDLCD == 1

void lcd_init(void);
void lcd_clear(void);

void lcd_print(int line, const char *msg);

#endif


#if CONFIG_GRLCD == 1

/* low level lcd access */
void lcd_clear_page(int page);
void lcd_data(unsigned char data);
void lcd_page(int i);
void lcd_column(int i);
void lcd_print_at(int col, int low_page, const char *str);

void lcd_clear_line(int line);
void lcd_logo_draw(void);

#elif CONFIG_OLDLCD == 1

/* some functions don't exist for the old LCD
   TODO: implement them if we continue to support? These functions
   probably don't match the old LCD's interface though...
 */

static inline void lcd_clear_page(int page) {}
static inline void lcd_data(unsigned char data) {}
static inline void lcd_page(int i) {}
static inline void lcd_column(int i) {}
static inline void lcd_print_at(int col, int low_page, const char *str) {}
static inline void lcd_clear_line(int line) {}
static inline void lcd_logo_draw(void) {}
#else

/* Add empty declarations when no LCD enabled */

static inline void lcd_init(void) {}
static inline void lcd_clear(void) {}
static inline void lcd_clear_page(int page) {}
static inline void lcd_data(unsigned char data) {}
static inline void lcd_page(int i) {}
static inline void lcd_column(int i) {}
static inline void lcd_print_at(int col, int low_page, const char *str) {}
static inline void lcd_clear_line(int line) {}
static inline void lcd_print(int line, const char *msg) {}
static inline void lcd_logo_draw(void) {}

#endif

#endif
