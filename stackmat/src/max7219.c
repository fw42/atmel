/**************************************************************
 * Selfmade Stackmat Display - (c) 2008 by Florian Weingarten *
 *                                                            *
 * This file is free software! published under the terms of   *
 * GNU general public license (GPL)                           *
 *                                                            *
 * firmware version 0.1 (first working published version)     *
 **************************************************************/

#include "max7219.h"
#include <avr/io.h>

#define PORT	PORTC
#define DDR	DDRC
#define DIN	PC2
#define LOAD	PC4
#define CLOCK	PD3

static void all_leds_off(void) {
	set_register_b(REG_DIG0, 0x0f);
	set_register_b(REG_DIG1, 0x0f);
	set_register_b(REG_DIG2, 0x0f);
	set_register_b(REG_DIG3, 0x0f);
	set_register_b(REG_DIG4, 0x0f);
	set_register_b(REG_DIG5, 0x0f);
	set_register_b(REG_DIG6, 0x0f);
	set_register_b(REG_DIG7, 0x0f);
}

void init_display(uint8_t intensity) {

	// set pins as output
	DDR |= ( 1<<DIN | 1<<LOAD | 1<<CLOCK );

	// set outputs as low
	PORT &= ~ ( 1<<DIN | 1<<LOAD | 1<<CLOCK );

	// set intensity
	set_register_b(REG_INTENSITY, intensity);

	// we want to see the first five digits
	set_register_b(REG_SCANLIMIT, 0x04);

	// decode mode for all digits
	set_register_b(REG_DECODE, 0xff);

	// initially, turn off all digits
	all_leds_off();

	// normal operation mode, no test mode
	set_register(DISPLAY_NORMALMODE);
	set_register(DISPLAY_NOTEST);
}

void set_register(uint16_t data) {

	uint8_t i;

	PORT &= ~( 1<<LOAD | 1<<DIN | 1<<CLOCK );

	for (i = 0; i < 16; i++) {
		if (data & 0x8000) {
			PORT |= (1<<DIN);
		} else {
			PORT &= ~(1<<DIN);
		}

		PORT |= (1<<CLOCK);
		PORT &= ~(1<<CLOCK);
		
		data <<= 1;
	}

	PORT |= (1<<LOAD);
	PORT &= ~(1<<LOAD);
}

