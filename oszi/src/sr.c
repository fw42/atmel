// vim:foldmethod=marker:foldmarker=//[[,//]]
/***************************************************************
 * (c) 2008 by Florian Weingarten <flo@hackvalue.de>           *
 * Published under the terms of the GNU General Public License *
 ***************************************************************/

#include <avr/io.h>
#include <util/delay.h>

#include "sr.h"

#define	PORT	PORTD
#define DDR	DDRD
#define STROBE	PD5
#define DATA	PD6
#define CLK	PD7

static void sr_send_byte(uint8_t byte) {
//[[
	for (uint8_t i = 0; i < 8; i++) {
        	if (byte & (1<<7)) {
			PORT |=  (1<<DATA);
		} else {
			PORT &= ~(1<<DATA);
		}

		// Clock togglen, damit Schieberegister Daten liest
		PORT |=  (1<<CLK);
		PORT &= ~(1<<CLK);

		byte <<= 1;
	}

} //]]

void sr_send(uint8_t byte1, uint8_t byte2) {
//[[
	sr_send_byte(byte1);
	sr_send_byte(byte2);

	// STROBE togglen, damit geladene Daten interpretiert werden
	PORT |=  (1<<STROBE);
	PORT &= ~(1<<STROBE);
} //]]

void init_sr(void) {
//[[
	// Clock, Data, OE und Strobe auf Output und Low
	DDR  |=  ( 1<<CLK | 1<<DATA | 1<<STROBE );
	PORT &= ~( 1<<CLK | 1<<DATA | 1<<STROBE );
} //]]
