// vim:foldmethod=marker:foldmarker=//[[,//]]

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "pt/pt.h"

#include "scope.h"
#include "sr.h"
#include "rs232.h"
#include "tables.h"
#include "adc.h"

#define MODES		5
#define MODE_SINE	0
#define MODE_SQUARE	1
#define MODE_TRIANGLE	2
#define MODE_SAWTOOTH	3
#define MODE_POTI	4

volatile uint8_t timer = 0;
volatile uint8_t wait_timer = 0;

uint8_t pwm_auto = 0;
volatile uint8_t dac1_mode = 1;
volatile uint8_t dac2_mode = 0;
volatile uint8_t dac1_byte = 0;
volatile uint8_t dac2_byte = 0;

uint8_t poti[4];
uint8_t selected_item = 0;

// Highspeed ISR für DA Output
ISR(TIMER0_COMPA_vect) {
//[[
	static uint8_t i,j;

	if(dac1_mode == MODE_SINE) {
		dac1_byte = pgm_read_byte(&(sinus[i]));
	} else if(dac1_mode == MODE_SQUARE) {
		dac1_byte = (i < 128) ? 0x00 : 0xFF;
	} else if(dac1_mode == MODE_TRIANGLE) {
		dac1_byte = (i < 128) ? i : -i;
	} else if(dac1_mode == MODE_SAWTOOTH) {
		dac1_byte = i;
	} else if(dac1_mode == MODE_POTI) {
		dac1_byte = poti[0] / 4;
	}

	if(dac2_mode == MODE_SINE) {
		dac2_byte = pgm_read_byte(&(sinus[j]));
	} else if(dac2_mode == MODE_SQUARE) {
		dac2_byte = (j < 128) ? 0x00 : 0xFF;
	} else if(dac2_mode == MODE_TRIANGLE) {
		dac2_byte = (j < 128) ? j : -j;
	} else if(dac2_mode == MODE_SAWTOOTH) {
		dac2_byte = j;
	} else if(dac2_mode == MODE_POTI) {
		dac2_byte = poti[1] / 4;
	}

	i += (poti[0]/4 ? ( poti[0] / 4 ) : 1);
	j += (poti[1]/4 ? ( poti[1] / 4 ) : 1);

	sr_send(dac1_byte, dac2_byte);
} //]]

static void update_leds(void) {
//[[	
	if(selected_item == 0) {

		LED1_PORT |=  (1<<LED1);
		LED2_PORT &= ~(1<<LED2);
		LED3_PORT &= ~(1<<LED3);

	} else if(selected_item == 1) {

		LED1_PORT &= ~(1<<LED1);
		LED2_PORT |=  (1<<LED2);
		LED3_PORT &= ~(1<<LED3);

	} else if(selected_item == 2) {

		LED1_PORT &= ~(1<<LED1);
		LED2_PORT &= ~(1<<LED2);
		LED3_PORT |=  (1<<LED3);

	}
} //]]

static void init_leds(void) {
//[[	
	// Output
	LED1_DDR |= (1<<LED1);
	LED2_DDR |= (1<<LED2);
	LED3_DDR |= (1<<LED3);
	LED4_DDR |= (1<<LED4);

	// Aus
	LED1_PORT &= ~(1<<LED1);
	LED2_PORT &= ~(1<<LED2);
	LED3_PORT &= ~(1<<LED3);
	LED4_PORT |=  (1<<LED4);

	update_leds();
} //]]

static void init_timer(void) {
//[[
	// Alle Timer2 Interrupts deaktivieren
	TIMSK2 = 0;

	// CTC mode
	TCCR2A |= (1<<WGM21);

	// Prescaler 1024
	TCCR2B |=  (1<<CS22 | 1<<CS20 | 1<<CS21);
	
	// Timer reset
	TCNT2 = 0;

	// 20 Mhz / 1024 / 100 = 195
	OCR2A = 195;
	OCR2B = 

	// clear Timer/Counter0 Output Compare A Match register
	TIFR2 |= (1<<OCF2A);
} //]]

static void init_da_timer(void) {
//[[
	// Alle Timer2 Interrupts deaktivieren
	TIMSK0 = 0;

	// CTC mode
	TCCR0A |= (1<<WGM01);

	// Prescaler 64
	TCCR0B |= ( 1<<CS02 );
	
	// Timer reset
	TCNT0 = 0;

	OCR0A = 1;

	// clear Timer/Counter0 Output Compare A Match register
	TIFR0 |= (1<<OCF0A);
	
	// Timer/Counter0 Output Compare Match A Interrupt Enable
	TIMSK0 = 1<<OCIE0A;
} //]]

static void init_taster(void) {
//[[	
	// Input
	TASTER1_DDR &= ~(1<<TASTER1);
	TASTER2_DDR &= ~(1<<TASTER2);
	TASTER3_DDR &= ~(1<<TASTER3);
	TASTER4_DDR &= ~(1<<TASTER4);

	// Pullups
	TASTER1_PORT |= (1<<TASTER1);
	TASTER2_PORT |= (1<<TASTER2);
	TASTER3_PORT |= (1<<TASTER3);
	TASTER4_PORT |= (1<<TASTER4);
} //]]

static void init_pwm(void) {
//[[
	DDRB |= (1<<PB1);

	// Alle Timer1 Interrupts deaktivieren
	TIMSK1 = 0;

	// Phase and Frequency Correct PWM
	TCCR1A |= (1<<WGM11);
	TCCR1B |= (1<<WGM12 | 1<<WGM13);

	// Clear on compare match, set on bottom
	TCCR1A |=  (1<<COM1A1);

	// Prescaler 256
	TCCR1B |= (1<<CS11 | 1<<CS10);
	
	// Timer reset
	TCNT1 = 0;

	// Foo
	ICR1 = 255;
	OCR1A = 1;

	// clear Timer/Counter0 Output Compare A Match register
	TIFR1 |= (1<<OCF1A);
} //]]

static void parse_keypress(uint8_t key) {
//[[
	if(key == 1) {

		selected_item = (selected_item + 1) % 3;
		update_leds();

	} else if(key == 2) {

		if(selected_item == 0) {
			dac1_mode = (dac1_mode + 1) % MODES;
		} else if(selected_item == 1) {
			dac2_mode = (dac2_mode + 1) % MODES;
		} else if(selected_item == 2) {
			pwm_auto = 1 - pwm_auto;
		}
	} else if(key == 3) {
		
		if(selected_item == 0 || selected_item == 1) {

			if(OCR0A < 255) {
				OCR0A++;
			}

		}

	} else if(key == 4) {

		if(selected_item == 0 || selected_item == 1) {

			if(OCR0A > 0) {
				OCR0A--;
			}

		}

	}
} //]]

static PT_THREAD(blink_thread_func(struct pt *thread)) {
//[[
	PT_BEGIN(thread);
	PT_WAIT_UNTIL(thread,timer==50);
	LED4_PORT ^= (1<<LED4);
	timer = 0;
	PT_END(thread);
} //]]

static PT_THREAD(timer_thread_func(struct pt *thread)) {
//[[
	static int8_t i=1;

	PT_BEGIN(thread);
	PT_WAIT_UNTIL(thread, TIFR2 & (1<<OCF2A));
	timer++;
	wait_timer++;
	TIFR2 |= (1<<OCF2A);
	PT_YIELD(thread);

	if(pwm_auto) {
		uint8_t max = (ICR1<255) ? (ICR1-1) : 254;

		if(OCR1A == 1) {
			i=1;
		} else if(OCR1A == max) {
			i=-1;
		}
	
		OCR1A = (((OCR1A + i) + 255) % 255);
	} else {
		OCR1A = poti[3];
	}

	ICR1 = poti[2];

	PT_END(thread);
} //]]

static PT_THREAD(poti_thread_func(struct pt *thread)) {
//[[
	static int8_t k=0,i=0;
	static uint16_t tmp=0;

	PT_BEGIN(thread);

	for(k=0; k<4; k++) {
	
		tmp=0;

		adc_set_source(k);

		for(i=0; i<5; i++) {
			adc_start();
			PT_WAIT_UNTIL(thread, ADCSRA & (1<<ADIF));
			tmp += adc_get_poti();
		}

		poti[k] = tmp / 5;
	}

	PT_END(thread);

} //]]

static PT_THREAD(debounce_thread_func(struct pt *thread)) {
//[[
	PT_BEGIN(thread);

	if(!(TASTER1_PIN & (1<<TASTER1))) {
		wait_timer = 0;
		PT_WAIT_UNTIL(thread, wait_timer >= 10);
		if(TASTER1_PIN & (1<<TASTER1)) {
			PT_YIELD(thread);
			parse_keypress(1);
		}
	}

	PT_YIELD(thread);

	if(!(TASTER2_PIN & (1<<TASTER2))) {
		wait_timer = 0;
		PT_WAIT_UNTIL(thread, wait_timer >= 10);
		if(TASTER2_PIN & (1<<TASTER2)) {
			PT_YIELD(thread);
			parse_keypress(2);
		}
	}

	PT_YIELD(thread);

	if(!(TASTER3_PIN & (1<<TASTER3))) {
		wait_timer = 0;
		PT_WAIT_UNTIL(thread, wait_timer >= 10);
		if(TASTER3_PIN & (1<<TASTER3)) {
			PT_YIELD(thread);
			parse_keypress(3);
		}
	}

	PT_YIELD(thread);

	if(!(TASTER4_PIN & (1<<TASTER4))) {
		wait_timer = 0;
		PT_WAIT_UNTIL(thread, wait_timer >= 10);
		if(TASTER4_PIN & (1<<TASTER4)) {
			PT_YIELD(thread);
			parse_keypress(4);
		}
	}

	PT_END(thread);

} //]]

int main() {
//[[

//[[
#ifdef UART
	init_uart();
#endif

	init_leds();
	init_sr();
	init_taster();
	init_adc();
	init_timer();
	init_pwm();
	init_da_timer();

	sei();

	sr_send(0x00,0x00);

#ifdef UART
	tx_byte('R');
	tx_byte('e');
	tx_byte('a');
	tx_byte('d');
	tx_byte('y');
	tx_byte('!');
	tx_byte('\n');
	tx_byte('\r');
#endif
//]]

	struct pt blink_thread, timer_thread, poti_thread, debounce_thread;

	PT_INIT(&blink_thread);
	PT_INIT(&timer_thread);
	PT_INIT(&poti_thread);
	PT_INIT(&debounce_thread);

	while(1) {
		PT_SCHEDULE(timer_thread_func(&timer_thread));
		PT_SCHEDULE(poti_thread_func(&poti_thread));
		PT_SCHEDULE(blink_thread_func(&blink_thread));
		PT_SCHEDULE(debounce_thread_func(&debounce_thread));
	}

	return 0;
} //]]

