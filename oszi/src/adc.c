// vim:foldmethod=marker:foldmarker=//[[,//]]

#include <avr/io.h>
#include "adc.h"

void init_adc(void) {
//[[
	// VCC als Referenzspannung mit C an AREF
	ADMUX &= ~(1<<REFS1);
	ADMUX |=  (1<<REFS0);

	// AD-Wandler einschalten und Prescaler = 64 einstellen (enstpricht 115 khz Wandlertakt)
	ADCSRA |= (1<<ADEN | 1<<ADPS0 | 1<<ADPS1 | 1<<ADPS2);
} //]]

// Poti auswählen
void adc_set_source(uint8_t s) {
//[[

	ADMUX &= ~(1<<MUX3);

	if(s == 3) {
		ADMUX &= ~( 1<<MUX0 | 1<<MUX1 | 1<<MUX2 );
	} else if(s == 2) {
		ADMUX &= ~( 1<<MUX1 | 1<<MUX2 );
		ADMUX |=  ( 1<<MUX0 );
	} else if(s == 1) {
		ADMUX &= ~( 1<<MUX0 | 1<<MUX2 );
		ADMUX |=  ( 1<<MUX1 );
	} else if(s == 0) {
		ADMUX &= ~( 1<<MUX2 );
		ADMUX |=  ( 1<<MUX0 | 1<<MUX1 );
	}

} //]]

// AD Wandlung starten
void adc_start(void) {
//[[
	ADCSRA |= (1<<ADSC);
} //]]

// Poti Wert zurück geben (in 8 Bit Auflösung)
uint8_t adc_get_poti(void) {
//[[
	return (ADCL | (ADCH<<8)) / 4;
} //]]

