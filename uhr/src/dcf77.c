// vim:foldmethod=marker:foldmarker=//[[,//]]
/***************************************************************
 * (c) 2008 by Florian Weingarten <flo@hackvalue.de>           *
 * Published under the terms of the GNU General Public License *
 ***************************************************************/

#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/parity.h>
#include "uhr.h"
#include "dcf77.h"

#define parity(a) parity_even_bit(a)

union DCF77_data_vector {
    uint64_t raw;
    struct {
        uint16_t reserved_bits:16;	// irgendwelche reservierten Bits (Wetterinfo, etc.)
        uint8_t timezone_transition:1;	// Zeitzonenwechsel?
        uint8_t cest:1;			// Sommerzeit?
        uint8_t cet:1;			// Winterzeit?
        uint8_t leapsecond:1;		// Schaltsekunde am Ende der aktuellen Stunde?
        uint8_t start:1;		// Startbit, ist immer gesetzt
        uint8_t minute:7;		// Minute (BCD)
        uint8_t minute_parity:1;	// Parity für Minute
        uint8_t hour:6;			// Stunde (BCD)
        uint8_t hour_parity:1;		// Parity für Stunde
        uint8_t day:6;			// Tage (BCD)
        uint8_t dow:3;			// Wochentag (BCD, beginnend bei Sonntag (0))
        uint8_t month:5;		// Monat (BCD)
        uint8_t year:8;			// Jahr (zweistellig (Offset auf 2000), BCD)
        uint8_t date_parity:1;		// Parity für Datum (Tag, Wochentag, Monat, Jahr)
    } bits;
} dcf77_data;

// Wieviele Bits wurden bisher empfangen?
static volatile uint8_t dcf77_bitcounter = 0;

// DCF77 Power Pin auf High
void dcf77_power_on(void) { //[[
	DCF77_VCC_PORT |=  (1<<DCF77_VCC);

	DCF77_PON_PORT |=  (1<<DCF77_PON);
	_delay_ms(50);
	DCF77_PON_PORT &= ~(1<<DCF77_PON);
} //]]

// DCF77 Power Pin auf Low
void dcf77_power_off(void) { //[[
	DCF77_PON_PORT |=  (1<<DCF77_PON);
	DCF77_VCC_PORT &= ~(1<<DCF77_VCC);
} //]]

// DCF77 Interrupt einschalten und fallende Flanke als Trigger
void enable_dcf77_interrupt(void) { //[[

	// Externer Interrupt an Pin INT0 bei FALLENDER Flanke
	EICRA |= 1<<ISC01;

	// Externen Interrupt an Pin INT0 aktivieren
	EIMSK |= 1<<INT0;

} //]]

// DCF77 Interrupt abschalten und Status LEDs ausschalten
void disable_dcf77_interrupt(void) { //[[

	// Mutex wieder freigeben, damit Quarz sich nicht aufhängt
	// wenn man im falschen Augenblick DCF77 ausgeschaltet hat
	dcf77_mutex = 0;

	// Externen Interrupt an Pin INT0 deaktivieren
	EIMSK &= ~(1<<INT0);

} //]]

// DCF77 Kram initialisieren (externen Interrupt, etc.)
void init_dcf77(uint8_t mode) { //[[

	// DCF77 VCC und PON auf Output
	DCF77_VCC_DDR	|= (1<<DCF77_VCC);
	DCF77_PON_DDR	|= (1<<DCF77_PON);
	
	// PON High (Modul abgeschaltet)
	DCF77_PON_PORT	|= (1<<DCF77_PON);

	if(mode == DCF77_FLAG_ON) {
		dcf77_power_on();
		enable_dcf77_interrupt();
	}

	// Timer1 (16 Bit) Normal Mode
	TCCR1A = 0;

	// Timer1 prescaler 256
	TCCR1B = 1<<CS12;

} //]]

// BCD kodierte Bitvektoren in unsigned integer umwandeln
static uint8_t bcd_to_uint8(uint8_t bcd) { //[[

	uint8_t ret = 0;
	uint8_t wertigkeit[] = {1,2,4,8,10,20,40,80};

	for(int8_t i=0; i<8; i++) {
		if(bcd & 1) {
			ret += wertigkeit[i];
		}
		bcd >>= 1;
	}

	return ret;

} //]]

// Interrupt Service Routine für DCF77 (externen Interrupt an INT0)
ISR(INT0_vect) { //[[

	LED_PORT ^= (1<<LED);
	
	uint16_t delta;

	// Timerwert speichern um festzustellen, wieviel Zeit vergangen ist
	delta = TCNT1;

	// Timer resetten
	TCNT1 = 0;

	// Testen ob steigende oder fallende Flanke den Interrupt ausgelöst hat
	// ISC00 ist genau dann gesetzt, wenn STEIGENDE Flanke auslöst
	if (EICRA & 1<<ISC00) {

		// Lange Zeit kein Flankenwechsel => neue Sendeminute startet
	        if (delta > 40000) {

			// Wenn alle Bits empfangen wurden ...
			if (dcf77_bitcounter == 59) {
				
				// Timer2 und Sekunden resetten
				TCNT2 = 0;

				dcf77_mutex = 1;
				zeit.seconds = 0;
				dcf77_mutex = 0;

				if (dcf77_data.bits.minute_parity == parity(dcf77_data.bits.minute)) {
					dcf77_mutex = 1;
					zeit.minutes = bcd_to_uint8(dcf77_data.bits.minute);
					dcf77_mutex = 0;
				} else {
					zeit.errorcode += 100;
				}

				if (dcf77_data.bits.hour_parity == parity(dcf77_data.bits.hour)) {
					dcf77_mutex = 1;
					zeit.hours = bcd_to_uint8(dcf77_data.bits.hour);
					dcf77_mutex = 0;
				} else {
					zeit.errorcode += 10;
				}

				if (dcf77_data.bits.date_parity == (
					parity(dcf77_data.bits.dow) ^
	                                parity(dcf77_data.bits.day) ^
					parity(dcf77_data.bits.month) ^
					parity(dcf77_data.bits.year)
				)) {
					dcf77_mutex = 1;
					zeit.dow = bcd_to_uint8(dcf77_data.bits.dow);
					zeit.dom = bcd_to_uint8(dcf77_data.bits.day);
					zeit.month = bcd_to_uint8(dcf77_data.bits.month);
					zeit.year = bcd_to_uint8(dcf77_data.bits.year);
					dcf77_mutex = 0;

				} else {
					zeit.errorcode += 1;
				}

				// Falls keine Fehler auftraten, Error code 0
				if(zeit.errorcode == 0) {
					if(dcf77_mode == DCF77_FLAG_HOUR) {
						// falls wir im Stundenmodus sind, Interrupt (und
						// Spannung für Empfänger) sofort abschalten nachdem
						// erfolgreich empfangen wurde
						disable_dcf77_interrupt();
						dcf77_power_off();
					}
				}

			}

			// Raw Data und Bitzähler zurücksetzen
			dcf77_bitcounter = 0;
			dcf77_data.raw = 0;

		}

	} else {

		// Falls delta zu klein, ignorieren
		// Prescaler 256 bei 8 MHz, d.h. 2187 entspricht 70ms, 7178 entspricht 230 ms,
		// ( ticks = ((F_CPU / 256) / 1000) * ms )
		if (delta > 2187 && delta < 7178) {
			// 4687 entspricht 150 ms
			if (delta >= 4687 && dcf77_bitcounter < 60) {
				dcf77_data.raw = dcf77_data.raw | (uint64_t) 1 << dcf77_bitcounter;
			}

			dcf77_bitcounter++;
		}
	}

	// ISC00 togglen, damit nächster Interrupt bei der jeweils
	// anderen Flanke ausgelöst wird
	EICRA ^= 1<<ISC00;

} //]]

