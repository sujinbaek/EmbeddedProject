/*
 * LCD CONTROL
 * Beaglebone Black
 * Author : Haegeon Jeong
 * Date : 2015.04.06
 * Description : LCD library for Embedded Software
 *
 * VSS : GND
 * VDD : 5V
 * V0 : contrast (3.3V)
 * RS : Register select (Command mode : 0, Character mode : 1)
 * RW : Signal for read or write (write mode is most used.) - Set to GND
 * E  : Enable or strobe pin. ( Enable : 1 )
 * D0~D3 : Data pin (CONNECT TO GPIO)
 * D4~D7 : Data pin (CONNECT TO GPIO)
 * A, K : Back light (A: VCC, K: GND, USE IF YOU WANT)
 */

#include <stdio.h>
#include <stdlib.h>

#define CMD_MODE "0"
#define CHR_MODE "1"

#define FIRST_LINE 0x80
#define SECOND_LINE 0xC0

static FILE **gpio_fp;

void lcd_send(unsigned char data, const char * mode)
{
	int i = 0;

	fprintf(gpio_fp[0], mode);
	fflush(gpio_fp[0]);

	for (i = 2; i < 10; i++)
	{
		fprintf(gpio_fp[i], "%d", (data&(0x1<<i-2)) >> i-2);
		fflush(gpio_fp[i]);
	}

	usleep(200);
	fprintf(gpio_fp[1], "1");
	fflush(gpio_fp[1]);
	usleep(500);
	fprintf(gpio_fp[1], "0");
	fflush(gpio_fp[1]);
	usleep(200);
}


void lcd_init(FILE **_gpio)
{
	gpio_fp = _gpio;
	lcd_send(0x38, CMD_MODE);
	lcd_send(0x16, CMD_MODE);
	lcd_send(0x0E, CMD_MODE);
	lcd_send(0x06, CMD_MODE);
	lcd_send(0x01, CMD_MODE);
}


void lcd_write(char * message)
{
//	lcd_send(line, CMD_MODE);
	for (*message; *message != '\0'; message++) {
		lcd_send((unsigned char)*message, CHR_MODE);
	}
}
