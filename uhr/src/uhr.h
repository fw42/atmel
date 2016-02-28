/***************************************************************
 * (c) 2008 by Florian Weingarten <flo@hackvalue.de>           *
 * Published under the terms of the GNU General Public License *
 ***************************************************************/

// DCF77 Modes
#define DCF77_FLAG_OFF		0
#define DCF77_FLAG_ON		1
#define DCF77_FLAG_HOUR		2

#define	TASTER_PORT		PORTC
#define TASTER_DDR		DDRC
#define TASTER_PIN		PINC
#define TASTER1			PC5
#define TASTER2			PC4
#define TASTER3			PB3
#define TASTER4			PC2

#define LED_PORT		PORTD
#define LED_DDR			DDRD
#define LED			PD6

#ifndef MAIN
extern struct {
	volatile uint8_t seconds;
	volatile uint8_t minutes;
	volatile uint8_t hours;
	volatile uint8_t dow;		// Wochentag
	volatile uint8_t dom;		// Monatstag
	volatile uint8_t month;
	volatile uint8_t year;		// Offset auf Jahr 2000
	volatile uint8_t errorcode;	// DFC77 Fehlercode
} zeit;

extern uint8_t dcf77_mode;
extern uint8_t dcf77_mutex;

extern uint8_t intensity;
#endif

void my_delay_ms(uint16_t);
void set_default_time(void);

