// vim:foldmethod=marker:foldmarker=//[[,//]]
/***************************************************************
 * (c) 2008 by Florian Weingarten <flo@hackvalue.de>           *
 * Published under the terms of the GNU General Public License *
 ***************************************************************/

#include <avr/io.h>
#include "max7219.h"

// Pin Definitionen, siehe Schaltplan
// (Ports und DDRs müssen unten im Code noch geändert werden)
#define DIN	PC1
#define LOAD	PC0
#define CLOCK	PD0

// Display Initialisierung (Paramtert: Display Helligkeit)
void init_max7219(uint8_t intensity) { //[[

	// Pins auf Output
	DDRC |= ( 1<<DIN | 1<<LOAD );
	DDRD |= ( 1<<CLOCK );

	// LED Intensität setzen
	// (wird intern vom MAX7219 per Pulsweitenmodulation geregelt)
	max7219_set_register_b(REG_INTENSITY, intensity);

	max7219_set_register_b(REG_SCANLIMIT, 0x05);

	// Dekodiermodus für alle Ziffern
	// (Segmente nicht einzeln ansteuern)
	max7219_set_register_b(REG_DECODE, 0xFF);

	// Alle Segmente ausschalten am Anfang
	max7219_all_digits_off();

	// Kein Test Modus
	max7219_set_register(DISPLAY_NORMALMODE);
	max7219_set_register(DISPLAY_NOTEST);

} //]]

// Alle Segmente ausschalten
void max7219_all_digits_off(void) { //[[
	for(int8_t i=0; i<8; i++) {
		max7219_set_register_b(REG_DIG0+i, 0x0f);
	}
} //]]

// Register setzen
// 2 Byte Daten, erstes Byte Adresse, zweites Byte Payload
// (siehe Datenblatt, set_register_b() Makro benutzen, das
// hier ist nur Backend)
void max7219_set_register(uint16_t data) { //[[

	uint8_t i;

	PORTC &= ~( 1<<LOAD | 1<<DIN );
	PORTD &= ~( 1<<CLOCK );

	for (i = 0; i < 16; i++) {
		if (data & 0x8000) {
			PORTC |=  (1<<DIN);
		} else {
			PORTC &= ~(1<<DIN);
		}

		// Clock togglen, damit MAX7219 Daten holt
		PORTD |=  (1<<CLOCK);
		PORTD &= ~(1<<CLOCK);
		
		data <<= 1;
	}

	// LOAD togglen, damit MAX7219 Daten interpretiert
	PORTC |=  (1<<LOAD);
	PORTC &= ~(1<<LOAD);
} //]]

