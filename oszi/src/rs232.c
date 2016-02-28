// vim:foldmethod=marker:foldmarker=//[[,//]]

/***************************************************************
 * (c) 2008 by Florian Weingarten <flo@hackvalue.de>           *
 * Published under the terms of the GNU General Public License *
 ***************************************************************/

#include <avr/io.h>
#include "rs232.h"

// UART initialisieren
void init_uart(void) {
//[[
	// Sender aktivieren
	UCSR0B = (1<<TXEN0);

	// 8 Daten Bits, 1 Stop Bit, kein Parity Bit
	UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);

	// Oberes Byte der Baudrate setzen
	UBRR0H = (uint8_t)(UBRR_VAL >> 8);

	// Unteres Byte der Baudrate setzen
	UBRR0L = (uint8_t)(UBRR_VAL);
} //]]

// Byte senden
void tx_byte(uint8_t data) {
//[[
	// Warten bis Sendebuffer leer ist
	while ( !( UCSR0A & (1<<UDRE0)) );

	// Daten in Buffer schreiben und damit Uebertragung erzwingen
	UDR0 = data;
} //]]

