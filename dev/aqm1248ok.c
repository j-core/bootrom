/* AQM1248test.c */
#include "lcd.h"

#include "lcd_font.h"

#define  delay_us(x)  {  unsigned  int  _dcnt;  \
       _dcnt  =  (x)*1;  \
       while(--_dcnt  !=  0)  \
       continue;  }

void  delay_ms(int  ms){
       int  i;
       for(i=0;  i<  4*ms;  i++)
               delay_us(1000);
}

typedef  unsigned  char  BYTE;

//  SPIポート
#define  bitSCK     (1<<30)
#define  bitSDO     (1<<31)
#define  bitCS      (1<<28)
#define  bitRS      (1<<29)
#define  bitDDR     (0xf<<24)
#define CMD			1
#define DATA		0

#include "board.h"
#define PORT (DEVICE_GPIO->value)

//****************************************
//  SPIマスタ初期化(MSSP使用)
//****************************************
void  SPIMsInit(void)  {
   PORT = bitSCK | bitSDO | bitRS | bitCS | bitDDR;
}

//****************************************
//  SPIマスタ送受信(MSSP使用)
//    IN:    txdat  マスタ送信データ
//    RET:  SPIRxData  マスタ受信データ
//****************************************
void SpiTransferByte(BYTE txdat, int cmd)  {

       int i;

       PORT = bitSCK | bitSDO | (cmd ? 0 : bitRS) | bitDDR;
       // delay_us(1);
       for (i=7; i>-1; i--) {
          PORT = ((1<<i) & txdat ? bitSDO : 0) | (cmd ? 0 : bitRS)          | bitDDR;
          PORT = ((1<<i) & txdat ? bitSDO : 0) | (cmd ? 0 : bitRS) | bitSCK | bitDDR;
       }
       PORT |= bitCS | bitDDR;
}

static void lcd_cmd(unsigned char cmd) {
  SpiTransferByte(cmd, 1);
}

void lcd_data(unsigned char data) {
  SpiTransferByte(data, 0);
}

void lcd_page(int i) {
#if CONFIG_GRLCD_FLIP == 1
  /* flip page numbers because pages count from the bottom up */
  i = 5 - i;
#endif
  lcd_cmd(0xB0 | i);
}

void lcd_column(int i) {
  lcd_cmd(0x10 | ((i >> 4) & 0xF));
  lcd_cmd(i & 0xF);
}

static const unsigned char lcd_init_cmds[] = {
  0xAE, /* display off */
#if CONFIG_GRLCD_FLIP == 0
  0xA0, /* ADC Select (Segment Driver Direction Select) -
         Normal display RAM columns */
#else
  0xA1, /* ADC Select (Segment Driver Direction Select) - Reverse
         Reverse display RAM columns */
#endif
  0xC8, /* Common output mode select - Reverse
         selects scan direction of COM output */
  0xA3, /* Set LCD bias 1/7 bias */
  0x2C, /* Power control set - 100 Booster circuit ON */
  0xFF, /* delay */
  0x2E, /* Power control set - 110 Voltage regulator ON */
  0xFF, /* delay */
  0x2F, /* Power control set - 111 Voltage follow circuit ON */
  0x23, /* Internal resistor ratio mode - 011 */
  0x81, /* Electronic volume mode set to adjust brightness */
  0x1C, /* Electronic volume register set 011100 */
  0xA4, /* Display all points - normal display
         Can force all points on. */
  0x40, /* Set RAM display start line address to 0 */
  0xA6, /* Display Normal (versus Reversed) */
  0xAF, /* display on */
  0x00 /* sentinal to stop loop */
};

void lcd_init(void) {
  const unsigned char *cmd = lcd_init_cmds;
  unsigned char c;
  //  SPI port initialize
  SPIMsInit();
  //  AQM1248  initialize
  while ((c = *cmd++)) {
    if (c == 0xff)
      delay_ms(2);
    else 
      lcd_cmd(c);
  }
}

void lcd_clear_page(int page) {
  int i;
  lcd_page(page);
  lcd_column(0);
  for (i = 0; i < 132; i++)
    lcd_data(0);
}

void lcd_clear(void) {
  int x;
  for (x = 0; x < 6; x++) {
    lcd_clear_page(x);
  }
}

void lcd_print_at(int col, int low_page, const char *str) {
  char c;
  int i;
  const struct lcd_char *l;
  const char *s;

  /* print all the chacters in the lower page, then print all in the
     upper page to avoid switching pages constantly  */
  lcd_page(low_page);
  lcd_column(col);
  s = str;
  while ((c = *s++)) {
    if (c >= '!' && c <= '~') {
      l = &lcd_font_chars[c - '!'];
      for (i = 0; i < sizeof(l->low_page); i++) {
        lcd_data(l->low_page[i]);
      }
    } else if (c == ' ') {
      for (i = 0; i < sizeof(l->low_page); i++) {
        lcd_data(0);
      }
    } else {
      /* Draw box for unknown chars */
      for (i = 0; i < sizeof(l->low_page)-1; i++) {
        lcd_data(0xFF);
      }
      lcd_data(0);
    }
  }

  lcd_page(low_page+1);
  lcd_column(col);
  s = str;
  while ((c = *s++)) {
    if (c >= '!' && c <= '~') {
      l = &lcd_font_chars[c - '!'];
      for (i = 0; i < sizeof(l->high_page); i++) {
        lcd_data(l->high_page[i]);
      }
    } else if (c == ' ') {
      for (i = 0; i < sizeof(l->high_page); i++) {
        lcd_data(0);
      }
    } else {
      /* Draw box for unknown chars */
      for (i = 0; i < sizeof(l->high_page)-1; i++) {
        lcd_data(0xFF);
      }
      lcd_data(0);
    }
  }
}

void lcd_clear_line(int line) {
  switch (line) {
  case 0:
    lcd_clear_page(1);
    lcd_clear_page(2);
    break;
  case 1:
    lcd_clear_page(3);
    lcd_clear_page(4);
    break;
  }
}

void lcd_print(int line, const char *str) {
  if (line < 0 || line > 1) return;
  lcd_clear_line(line);
  int page = line == 0 ? 1 : 3;
  lcd_print_at(4, page, str);
}
