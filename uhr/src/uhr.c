// vim:foldmethod=marker:foldmarker=//[[,//]]
/***************************************************************
 * (c) 2008 by Florian Weingarten <flo@hackvalue.de>           *
 * Published under the terms of the GNU General Public License *
 ***************************************************************/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include "max7219.h"
#include "dcf77.h"

#define MAIN
#include "uhr.h"

// In welchem Sekundentakt soll der EEPROM Speicher
// abgeglichen werden (falls nötig)?
#define EEPROM_REFRESH_SECONDS	10

// Standardwerte die beim Programmieren in den EEPROM
// Speicher geschrieben werden
#define DEFAULT_INTENSITY	3
#define DEFAULT_DCF77		DCF77_FLAG_ON

// Flags für zweiten Display
#define MODES			4
#define MODE_TIME		0
#define MODE_SECONDS		1
#define MODE_MONTH		2
#define MODE_YEAR		3

struct {
	volatile uint8_t seconds;
	volatile uint8_t minutes;
	volatile uint8_t hours;
	volatile uint8_t dow;		// Wochentag
	volatile uint8_t dom;		// Monatstag
	volatile uint8_t month;
	volatile uint8_t year;		// Offset auf Jahr 2000
	volatile uint8_t errorcode;	// DCF77 Fehlercode
} zeit;

// Daten im EEPROM Speicher (Standardwerte werden von Compiler gesetzt
// und einmalig bei der Programmierung per ISP in den Chip geladen)
uint8_t eeprom_intensity	EEMEM = DEFAULT_INTENSITY;
uint8_t eeprom_dcf77		EEMEM = DEFAULT_DCF77;

// Flag für zweites Display
uint8_t display_mode;

// Flag für DCF77 Modus (Stundenmodus, Permanentmodus und Abgeschaltet)
uint8_t dcf77_mode = 1;
uint8_t dcf77_changed = 0;
uint8_t dcf77_mutex = 0; // Greift DCF77 gerade auf Datum zu? Für Quarz blockieren!

// Display Intensität und Status für LEDs (an/aus)
uint8_t intensity;

// Lohnt es sich den EEPROM auf Aktualität zu testen?
uint8_t eeprom_data_maybe_changed = 0;

// Wie lange soll (in Sekunden) gewartet werden, bis die Zeit
// wieder upgedated wird? (z.B. für Intesitätssteuerung, etc., damit
// Wert kurz sichtbar bleibt)
uint8_t wait = 0;

// Werte aus EEPROM Speicher laden
static void read_values_from_eeprom(void) { //[[

	// Daten lesen
	intensity	= eeprom_read_byte(&eeprom_intensity);
	dcf77_mode	= eeprom_read_byte(&eeprom_dcf77);

	// Testen ob Daten überhaupt Sinn ergeben oder ob EEPROM eventuell
	// falsch initialisiert (oder kaputt?) ist

	if(intensity > 15) {
		intensity = DEFAULT_INTENSITY;
	}

	if(dcf77_mode > 2) {
		dcf77_mode = DEFAULT_DCF77;
	}
} //]]

// Zeit auf LED Display anzeigen
static void display_time(void) { //[[

	// Decode Mode für alle Ziffern
	max7219_set_register_b(REG_DECODE, 0xFF);

	if(display_mode == MODE_TIME) {

		// Stunde (Punkt von Einerstelle jede zweite Sekunde an)
		max7219_set_register_b(REG_DIG0, zeit.hours / 10);
		max7219_set_register_b(REG_DIG1, (zeit.hours % 10));
	
		// Doppelpunkt
		if(zeit.seconds % 2) {
			max7219_set_register_b(REG_DIG4, 1<<7);
			max7219_set_register_b(REG_DIG5, 1<<7);
		} else {
			max7219_set_register_b(REG_DIG4, 0x0F);
			max7219_set_register_b(REG_DIG5, 0x0F);
		}

		// Minute
		max7219_set_register_b(REG_DIG2, zeit.minutes / 10);
		max7219_set_register_b(REG_DIG3, zeit.minutes % 10);

	} else if(display_mode == MODE_MONTH) {

		// Monatstag (mit Punkt nach Einerziffer)
		max7219_set_register_b(REG_DIG0, zeit.dom / 10);
		max7219_set_register_b(REG_DIG1, (zeit.dom % 10) | (1<<7));

		// Monat
		max7219_set_register_b(REG_DIG2, zeit.month / 10);
		max7219_set_register_b(REG_DIG3, (zeit.month % 10) | (1<<7));

	} else if(display_mode == MODE_YEAR) {

		// Jahr
		max7219_set_register_b(REG_DIG0, (zeit.year) / 1000 + 2);
		max7219_set_register_b(REG_DIG1, (zeit.year % 1000) / 100);
		max7219_set_register_b(REG_DIG2, (zeit.year & 100) / 10);
		max7219_set_register_b(REG_DIG3, (zeit.year % 10));

	} else if(display_mode == MODE_SECONDS) {
	
		// Sekunden
		max7219_set_register_b(REG_DIG0, 0x0F);
		max7219_set_register_b(REG_DIG1, zeit.seconds / 10);
		max7219_set_register_b(REG_DIG2, zeit.seconds % 10);
		max7219_set_register_b(REG_DIG3, 0x0F);

	}

} //]]

// Ist das angegebene Jahr ein Schaltjahr?
static uint8_t schaltjahr(uint8_t year_offset) { //[[
	// Test eigentlich nicht 100% korrekt, aber für die nächsten 82 Jahre
	// ist er korrekt genug :-)
	return ((2000 + year_offset) % 4 == 0);
} //]]

// Wieviele Tage hat ein Monat in einem Jahr? (Schaltjahre beachten!)
static uint8_t get_days_of_month(uint8_t month, uint8_t year) { //[[
	uint8_t days[13] = {0,31,28,31,30,31,30,31,31,30,31,30,31};

	if(month == 2) {
		return ( schaltjahr(year) ? 29 : 28 );
	}

	return days[month];
} //]]

// Zeit über Uhrenquarz updaten, falls Empfänger mal ausfällt
static void manual_update_time(void) { //[[
	
	zeit.seconds++;

	if(zeit.seconds == 60) {
		zeit.seconds = 0;
		zeit.minutes++;
	}

	if(zeit.minutes == 60) {
		zeit.minutes = 0;
		zeit.hours++;
	}

	if(zeit.hours == 24) {
		zeit.hours = 0;
		zeit.dom++;
	}

	uint8_t days = get_days_of_month(zeit.month, zeit.year);
	if(zeit.dom == days+1) {
		zeit.dom = 1;
		zeit.month++;
	}

	if(zeit.month == 12+1) {
		zeit.month = 1;
		zeit.year++;
	}

} //]]

// ms Milisekunden lang warten (busy loop)
void my_delay_ms(uint16_t ms) { //[[
	while(ms--) {
		_delay_ms(1);
	}	
} //]]

// Taster entprellen (Taster sind Active-Low)
static uint8_t debounce(uint8_t pin) { //[[

	// Falls Taster gedrückt ist ...
	if ( ! (TASTER_PIN & (1 << pin)) ) {
	
		// ... etwas warten ...
		my_delay_ms(100);

		// falls Taster losgelassen wurde
		if ( TASTER_PIN & (1 << pin) ) {
			my_delay_ms(50);
			return 1;
		} else {

			// Wird Taster eine Sekunde lang festgehalten?
			for(int8_t i=0; i<9; i++) {
				my_delay_ms(100);
				if(TASTER_PIN & (1<<pin)) {
					return 1;
				}
			}
			
			// ... dann 2 ausgeben
			return 2;

		}
	}

	return 0;

} //]]

// Taster initialisieren
static void init_taster(void) { //[[

	// Taster auf Input
	TASTER_DDR  &= ~(1<<TASTER1 | 1<<TASTER2 | 1<<TASTER3 | 1<<TASTER4);

	// Pullup Widerstände aktivieren
	TASTER_PORT |=  (1<<TASTER1 | 1<<TASTER2 | 1<<TASTER3 | 1<<TASTER4);

} //]]

// LED initialisieren
static void init_led(void) { //[[
	// LED auf Output und High (off)
	LED_DDR  |=  (1<<LED);
	LED_PORT &= ~(1<<LED);
} //]]

// 32.768 kHz Uhrenquarz als Timer2 initialisieren
static inline void init_quarz(void) { //[[

	// Output Compare Match Interrupt und Overflow Interrupt deaktivieren
	TIMSK2 &= ~( 1<<OCIE2A | 1<<TOIE2 );

    	// 32.768 kHz Uhrenquarz als externen Timer
	ASSR |= 1<<AS2;

	// Output Compare Match Modus
	TCNT2 = 0;
	OCR2B = 0;

	// prescaler 128, genau 256 Inkrementierungen für eine Sekunde (bei 32.768 kHz)
	TCCR2B |=  ( 1<<CS22 | 1<<CS20 );
	TCCR2B &= ~( 1<<CS21 );

	// Warten bis nichtmehr busy
	while ( ASSR & ( 1<<TCN2UB | 1<<OCR2BUB | 1<<TCR2BUB ));

	// Interrupt Flags löschen
	TIFR2 |= (1<<OCF2B) | (1<<TOV2);

	// Counter2 Overflow Interrupt Enable
	TIMSK2 |= (1<<TOIE2);

} //]]

// Timer2 Overflow Interrupt (Uhrenquarz Sekundentakt)
ISR(TIMER2_OVF_vect) { //[[

	if(!dcf77_mutex) {
		manual_update_time();
	}

	// Falls wir im Stundenmodus für DCF77 sind, bei jeder
	// vollen Stunde (+1) den DCF77 Empfänger aktivieren
	// (und bei erfolgreichem Empfang sofort wieder deaktivieren,
	// siehe dcf77.c)
	if(dcf77_mode == DCF77_FLAG_HOUR && zeit.minutes == 0 && zeit.seconds >= 45) {
		dcf77_power_on();
		enable_dcf77_interrupt();
	}

	if(wait) {
		wait--;
	} else {
		display_time();
	
		if(dcf77_changed) {
			if(dcf77_mode != DCF77_FLAG_ON) {
				dcf77_power_off();
				disable_dcf77_interrupt();
			} else {
				dcf77_power_on();
				enable_dcf77_interrupt();
			}
			dcf77_changed = 0;
		}
	}

	// Alle paar Sekunden EEPROM Inhalt abgleichen (falls nötig!)
	if(eeprom_data_maybe_changed && zeit.seconds % EEPROM_REFRESH_SECONDS == 0) {

		if(eeprom_read_byte(&eeprom_intensity) != intensity) {
			eeprom_write_byte(&eeprom_intensity, intensity);
		}

		if(eeprom_read_byte(&eeprom_dcf77) != dcf77_mode) {
			eeprom_write_byte(&eeprom_dcf77, dcf77_mode);
		}	

		eeprom_data_maybe_changed = 0;
	
	}

} //]]

// Zeiteinstellmenue
static void menu(void) { //[[

	max7219_set_register_b(REG_DECODE, 0xFF);

	uint8_t current_object = 0;
	uint8_t display[4];

	while(1) {
		wait = 0xFF;
		
		if(current_object < 2) {
			display[0] = zeit.hours / 10;
			display[1] = zeit.hours % 10;
			display[2] = zeit.minutes / 10;
			display[3] = zeit.minutes % 10;
		} else if(current_object < 4) {
			display[0] = zeit.dom / 10;
			display[1] = (zeit.dom % 10) | (1<<7);
			display[2] = zeit.month / 10;
			display[3] = (zeit.month % 10) | (1<<7);
		} else if(current_object == 4) {
			display[0] = 2;
			display[1] = zeit.year / 100;
			display[2] = (zeit.year % 100) / 10;
			display[3] = zeit.year % 10;
		} else {
			break;
		}

		if(debounce(TASTER1) && current_object > 0) {
			current_object--;
		}

		if(debounce(TASTER2)) {
			current_object++;
		}

		int8_t add=0;
		if(debounce(TASTER3)) {
			add=1;
		}
		if(debounce(TASTER4)) {
			add=-1;
		}

		if(current_object == 0) {

			zeit.hours = ((zeit.hours + add) + 24) % 24;

		} else if(current_object == 1) {

			zeit.minutes = ((zeit.minutes + add) + 60) % 60;

		} else if(current_object == 2) {

			uint8_t days = get_days_of_month(zeit.month, zeit.year);
			zeit.dom = ((zeit.dom + add) + (days+1)) % (days+1);
			if(zeit.dom == 0) {
				if(add==1) {
					zeit.dom = 1;
				} else if(add == -1) {
					zeit.dom = days;
				}
			}

		} else if(current_object == 3) {

			zeit.month = ((zeit.month + add) + 13) % 13;
			if(zeit.month == 0) {
				if(add==1) {
					zeit.month = 1;
				} else if(add==-1) {
					zeit.month = 12;
				}
			}

		} else if(current_object == 4) {

			if(!(zeit.year == 0 && add == -1)) {
				zeit.year += add;
			}

		}

		// Aktuelles Objekt blinken lassen (im Viertelsekundentakt)
		if(TCNT2 < 64 || ( TCNT2 >= 128 && TCNT2 < 192 ) ) {

			for(int8_t i=0; i<4; i++) {
				max7219_set_register_b(REG_DIG0+i, display[i]);
			}

		} else if(TCNT2 > 64 || TCNT2 > 192) {

			if(current_object != 4) {

				// 0 0
				// 1 2
				// 2 0
				// 3 2
				uint8_t i = 2 * ((current_object) % 2);
	
				max7219_set_register_b(REG_DIG0+i,   0x0F);
				max7219_set_register_b(REG_DIG0+i+1, 0x0F);

			} else if(current_object==4) {
				// Objekt 4 ist Jahreszahl, also alle Ziffern blinken
				max7219_set_register_b(REG_DIG0, 0x0F);
				max7219_set_register_b(REG_DIG1, 0x0F);
				max7219_set_register_b(REG_DIG2, 0x0F);
				max7219_set_register_b(REG_DIG3, 0x0F);

			}
		}

	}

	display_time();
	wait = 0;

	my_delay_ms(100);
} //]]

void set_default_time(void) {
//[[
	// 01.01.2008
	zeit.dom = 1;
	zeit.month = 1;
	zeit.year = 8;
} //]]

int main(void) {

	read_values_from_eeprom();

	init_max7219(intensity);

	init_taster();
	init_quarz();
	init_led();
	init_dcf77(dcf77_mode);

	set_default_time();

	// Global Interrupts aktivieren
	sei();

	// Main Loop
	while(1) {

		// Taster 1: Anzeigemodus für Display
		if(debounce(TASTER1)) { //[[
			display_mode = (display_mode + 1) % MODES;
			eeprom_data_maybe_changed = 1;
			display_time();
		} //]]

		// Taster 2: Zeiteinstellmenue
		if(debounce(TASTER2)) { //[[
			menu();
		} //]]

		// Taster 3: DCF77 Mode
		if(debounce(TASTER3)) { //[[
			dcf77_mode = (dcf77_mode + 1) % 3;
			dcf77_changed = 1;
			eeprom_data_maybe_changed = 1;

			max7219_set_register_b(REG_DECODE, 0xF0);

			if(dcf77_mode == DCF77_FLAG_OFF) {
				
				max7219_set_register_b(REG_DIG0, 0xFF & ~(1<<7 | 1<<0 ));		// O
				max7219_set_register_b(REG_DIG1, 0xFF & ~(1<<7 | 1<<5 | 1<<3 | 1<<4));	// F
				max7219_set_register_b(REG_DIG2, 0xFF & ~(1<<7 | 1<<5 | 1<<3 | 1<<4));	// F
				max7219_set_register_b(REG_DIG3, 0x00);					// 

			} else if(dcf77_mode == DCF77_FLAG_HOUR) {

				max7219_set_register_b(REG_DIG0, 0xFF & ~(1<<7 | 1<<6 | 1<<3));		// H
				max7219_set_register_b(REG_DIG1, 1<<2 | 1<<4 | 1<<3 | 1<<0);		// o
				max7219_set_register_b(REG_DIG2, 1<<2 | 1<<3 | 1<<4);			// u
				max7219_set_register_b(REG_DIG3, 1<<2 | 1<<0);				// r

			} else if(dcf77_mode == DCF77_FLAG_ON) {

				max7219_set_register_b(REG_DIG0, 0xFF & ~(1<<7 | 1<<0));		// O
				max7219_set_register_b(REG_DIG1, 0xFF & ~(1<<7 | 1<<0 | 1<<3));		// N
				max7219_set_register_b(REG_DIG2, 0x00);					// 
				max7219_set_register_b(REG_DIG3, 0x00);					// 

			}
			
			wait = 2;

		} //]]

		// Taster 4: Display Intensität und LED toggle
		//[[
		// Intensität des Display erhöhen und Wert in EEPROM Speicher ablegen
		if(debounce(TASTER4)) {

			// Intensität geht eigentlich bis 16, aber dann ist Strom durch LEDs
			// zu hoch (verbauter Widerstand zu klein), daher nur bis einschliesslich 7
			// => Strom ungefähr 40 mA * (15 / 32) =~ 18.75 mA
			intensity = (intensity + 1) % 8;
			eeprom_data_maybe_changed = 1;
			max7219_set_register_b(REG_INTENSITY, intensity);
			max7219_set_register_b(REG_DECODE, 0xFF);
			max7219_set_register_b(REG_DIG0, 10);
			max7219_set_register_b(REG_DIG1, intensity / 10);
			max7219_set_register_b(REG_DIG2, intensity % 10);
			max7219_set_register_b(REG_DIG3, 10);
			wait=2;

		} //]]

	}

	return 0;
}

