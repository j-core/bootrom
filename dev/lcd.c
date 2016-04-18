#include "lcd.h"
#include "board.h"

#define PORT (DEVICE_GPIO->value)
#define SDA 0x00000001
#define SCL 0x00000002
#define ID 0x7c

#define I2CSPEED 155
static void i2c_delay(int n) { volatile int i; for (i=0; i < I2CSPEED * n; i++) {}; }
static int read_SCL(void) { PORT |= SCL; return 1; } // Set SCL as input and return current level of line, 0 or 1
static int read_SDA(void) { PORT |= SDA; return PORT & SDA ? 1 : 0; } // Set SDA as input and return current level of line, 0 or 1
static void clear_SCL(void) { PORT &= ~SCL; } // Actively drive SCL signal low
static void clear_SDA(void) { PORT &= ~SDA; } // Actively drive SDA signal low

static unsigned char cmds[] = { 0x38, 0x39, 0x14, 0x70, 0x56, 0x6c, 0x38, 0x0c, 0x01, 0x00 };

static void init_i2c()
{
	PORT |= SDA | SCL;
}

static int send_i2c(int start, int stop, unsigned char x)
{
	int b;
	int ack;

	if (start) { clear_SDA(); i2c_delay(2); }

	for (b=7; b>-1; b--) {
		clear_SCL();
		if (x & (1<<b)) read_SDA(); else clear_SDA();
		i2c_delay(1);
		read_SCL();
		i2c_delay(2);
		clear_SCL();
		i2c_delay(1);
	}
	PORT |= SDA;
	i2c_delay(1);
	read_SCL();
	i2c_delay(1);
	ack=read_SDA();
	i2c_delay(1);
	clear_SCL();
	i2c_delay(1);

	if (stop) { clear_SDA(); i2c_delay(2); read_SCL(); i2c_delay(1); read_SDA(); }
	return ack;
}

void
lcd_init() {
	int i;

	for (i=0; i<2; i++) {  /* yes, we do this twice... just be sure */
		init_i2c();

		i2c_delay(100);

		for (i=0; cmds[i]; i++) {
			send_i2c(1, 0, 0x7c);
			send_i2c(0, 0, 0x80);
			send_i2c(0, 1, cmds[i]);
			i2c_delay(100);
		}
	}
}

void
lcd_clear() {
	init_i2c();
	i2c_delay(100);

	send_i2c(1, 0, 0x7c);
	send_i2c(0, 0, 0x80);
	send_i2c(0, 1, 0x01); /* clear command */
	i2c_delay(100);
}

void
lcd_print(int line, const char *msg) {
	int i;

	init_i2c();

	send_i2c(1, 0, 0x7c);
	send_i2c(0, 0, 0x80);
	send_i2c(0, 0, line ? 0xc0 : 0x80); /* 0=Line 1, else line 2 */
	for (i=0; msg[i]; i++) {
		send_i2c(0, 0, 0xc0);
		send_i2c(0, msg[i+1] ? 0 : 1, msg[i]);
	}
	i2c_delay(100);
}	
