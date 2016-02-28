// vim:foldmethod=marker:foldmarker=//[[,//]]

/**************************************************************
 * Selfmade Stackmat Display - (c) 2008 by Florian Weingarten *
 *                                                            *
 * This file is free software! published under the terms of   *
 * GNU general public license (GPL)                           *
 *                                                            *
 * firmware version 0.1 (first working published version)     *
 **************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "max7219.h"
#include "rs232.h"

// How big should the buffer for times be?
// (there is no buffer overflow, if there are TIMES+n times,
// the first n times get kicked out
// not sure how big this number can be. all times are stored
// in RAM and ATMega88 does have 1KB of RAM (where some program
// code, variables and stuff also resides)
#define TIMES			200

// how bright should the display be by default?
#define DEFAULT_INTENSITY	5

// Pin defines
//[[

#define BUTTON0_PORT	PORTB
#define BUTTON0_DDR	DDRB
#define BUTTON0_PIN	PINB
#define BUTTON0		PB6

#define BUTTON1_PORT	PORTB
#define BUTTON1_DDR	DDRB
#define BUTTON1_PIN	PINB
#define BUTTON1		PB7

#define BUTTON2_PORT	PORTD
#define BUTTON2_DDR	DDRD
#define BUTTON2_PIN	PIND
#define	BUTTON2		PD5

#define LED_PORT	PORTD
#define LED_DDR		DDRD
#define LED0		PD2
#define LED1		PD3
#define LED2		PD4

#define LIGHT_PORT	PORTC
#define LIGHT_DDR	DDRC
#define LIGHT		PC5
//]]

// Checksum for stackmat time (sum of all digit plus 64)
#define checksum(data)		( (data[1]-'0')+(data[2]-'0')+(data[3]-'0')+(data[4]-'0')+(data[5]-'0')+64 )

// Check if a byte is a valid state
#define state_is_valid(s)	(s=='I' || s=='A' || s==' ' || s=='S' || s=='L' || s=='R' || s=='C')

// Append data to array (and eventually drop old data)
#define append(array, length, item)	{ for(uint8_t i=0; i<length-1; i++) { array[i] = array[i+1]; };  array[length-1] = item; }

// Buffer for the last seven received bytes
// state, min, sec1, sec2, msec1, msec2, checksum
static uint8_t buf[7] = {0,0,0,0,0,0,0};

static uint16_t last_correct_decoded_time;
static uint8_t intensity = DEFAULT_INTENSITY;

static uint8_t num_bytes;		// how many bytes have been received since the last correct decoding?
static uint8_t blink;			// status variable for LED blinking
static uint8_t state;			// which state is the timer in?
static uint8_t time_already_saved;	// has the current time already been saved to times[]?

static uint16_t times[TIMES];		// buffer for saved times

// Convert last correct decoded time into an uint16
static uint16_t time_to_uint16(void) { //[[
	return ( (buf[5]-'0') + 10*(buf[4]-'0') + 100*(buf[3]-'0') + 1000*(buf[2]-'0') + 6000*(buf[1]-'0'));
} //]]

// _delay_ms() workaround (see <utils/delay.h> for more info)
static void my_delay_ms(uint16_t ms) { //[[
	while(ms--) {
		_delay_ms(1);
	}
} //]]

// update the display and the status LEDs according to the content of buf[]
// (gets called by USART interrupt routine)
static void parse_data(void) { //[[

	if(buf[0] == 'I') {
		time_already_saved = 0;
	}

	// if left stackmat pad is pressed, turn on its LED
	if(buf[0] == 'L' || buf[0] == 'C' || buf[0] == 'A') {
		LED_PORT |=  (1<<LED0);
	} else {
		LED_PORT &= ~(1<<LED0);
	}

	// if right stackmat pad is pressed, turn on its LED
	if(buf[0] == 'R' || buf[0] == 'C' || buf[0] == 'A') {
		LED_PORT |=  (1<<LED2);
	} else {
		LED_PORT &= ~(1<<LED2);
	}

	// if left and right where pressed long enough, enable "ready"-LED
	if(buf[0] == 'A') {
		LED_PORT |=  (1<<LED1);
	} else if(buf[0] != ' ' && !blink)  {
		LED_PORT &= ~(1<<LED1);
	}

	// if timer is running, do some blinking
	if(buf[0] == ' ' || blink) {
		if(!blink) {
			LED_PORT |= (1<<LED1);
			blink = 1;
		}
		LED_PORT ^= (1<<LED1);
	}

	// stop blinking if timer has stopped
	// this is necessary because 'L', 'R' and 'C' override ' ', but
	// the timer should blink while the time is running, even if one pad is pressed
	if(buf[0] == 'S' || buf[0] == 'C' || buf[0] == 'I') {
		LED_PORT &= ~(1<<LED1);
		blink = 0;
	}

	// diplay time
	for(uint8_t i=1; i<=5; i++) {
		uint8_t value = (buf[i] - '0') % 10;

		// no leading zeros
		if(!(i == 1 && value == 0) && !(i == 2 && ((buf[1] - '0') % 10) == 0 && value == 0)) {

			if(i==1 || i==3) {
				value |= (1<<7);
			};

			set_register_b(REG_DIG0+(i-1), value);

		} else {

			// Turn digit off
			set_register_b(REG_DIG0+(i-1), 0x0F);

		}
	}

	state = buf[0];
} //]]

// debounce buttons (active low)
static uint8_t debounce(volatile uint8_t *port, uint8_t pin) { //[[

	// if button is pressed ...
	if ( ! (*port & (1 << pin)) ) {
	
		// ... wait some time ...
		my_delay_ms(100);

		// if button has been released
		if ( *port & (1 << pin) ) {
			my_delay_ms(50);
			return 1;
		} else {

			// if button is released within the next second ...
			for(int8_t i=0; i<10; i++) {
				my_delay_ms(100);
				if(*port & (1<<pin)) {
					return 1;
				}
			}
			
			// otherwise ...
			return 2;

		}
	}

	return 0;

} //]]

// ISR for UART receive
ISR(USART_RX_vect) { //[[

	// read data (this HAS to be done!! see datasheet!)
	uint8_t c = UDR0;

	// if the current stackmat packet is not yet completed,
	// append the payload to the buffer
	if(c != 10 && c != 13) {
		append(buf,7,c);
		num_bytes++;
	}

	// check if data was correct, if yes, update the display...
	if(num_bytes >= 7 && checksum(buf) == buf[6] && state_is_valid(buf[0])) {
		num_bytes = 0;
		last_correct_decoded_time = time_to_uint16();
		parse_data();
	}
} //]]

// initialize buttons
static void init_buttons(void) { //[[

	// Set button pins as input
	BUTTON0_DDR &= ~(1<<BUTTON0);
	BUTTON1_DDR &= ~(1<<BUTTON1);
	BUTTON2_DDR &= ~(1<<BUTTON2);
	
	// Activate internal button pullup resistors
	// (buttons are active low, see schematic)
	BUTTON0_PORT |= (1<<BUTTON0);
	BUTTON1_PORT |= (1<<BUTTON1);
	BUTTON2_PORT |= (1<<BUTTON2);

} //]]

// initialize leds
static void init_leds(void) { //[[
	
	// set LED pins as output and low (off)
	LED_DDR  |=  ( 1<<LED0 | 1<<LED1 | 1<<LED2 );
	LED_PORT &= ~( 1<<LED0 | 1<<LED2 );

	// set case light to output and high (off)
	LIGHT_DDR  |= ( 1<<LIGHT );
	LIGHT_PORT |= ( 1<<LIGHT );
} //]]

// toggle LED n times (with pause between the toggles)
static void led_blink(uint8_t n) { //[[
	for(uint8_t i=0; i<n; i++) {
		LED_PORT &= ~(1<<LED1);
		my_delay_ms(50);
		LED_PORT |=  (1<<LED1);
		my_delay_ms(150);
	}
} //]]

// calculate the average of the times in times[]
static uint16_t average(void) { //[[

	uint32_t avg=0;
	uint8_t num=0;

	for(uint8_t i=0; i<TIMES; i++) {
		if(times[i]) {
			avg += times[i];
			num++;
		}
	}

	if(num) {
		return (uint16_t)(avg / num);
	} else {
		return 0;
	}
} //]]

// show time on display
static void display_uint16_time(uint16_t time) { //[[

//	all_leds_off();

	uint16_t rem = time%6000;

	if(time/6000 != 0) {
		set_register_b(REG_DIG0, time/6000 | (1<<7));		// activate "dot"
	} else {
		set_register_b(REG_DIG0, 0x0F);
	}

	set_register_b(REG_DIG1, rem/1000);
	set_register_b(REG_DIG2, (rem%1000)/100 | (1<<7));	// activate "dot"
	set_register_b(REG_DIG3, (rem%100)/10);
	set_register_b(REG_DIG4, (rem%10));

} //]]

int main(void) {

	init_buttons();
	init_leds();
	init_display(intensity);
	init_uart();

	uart_interrupt_enable();

	sei();

	// main loop
	while(1) {

		// while button 1 is pressed, increment display brightness (mod 16)
		uint8_t button_status = debounce(&BUTTON0_PIN, BUTTON0);
		if(button_status == 1) {
			intensity = (intensity + 1) % 16;
			set_register_b(REG_INTENSITY, intensity);
		} else if(button_status == 2) {
			LIGHT_PORT ^= (1<<LIGHT);
		}

		// if timer is stopped (but not yet resetted) ...
		if(state == 'S') {
			
			// ... and button 0 has been pressed (and released),
			// save the current time in the times array (only once!)
			if(debounce(&BUTTON1_PIN, BUTTON1) && !time_already_saved) {
				uart_interrupt_disable();
				append(times, TIMES, last_correct_decoded_time);
				led_blink(2);
				time_already_saved = 1;
				uart_interrupt_enable();
			}

		}

		// if timer has been stopped or reset ...
		if(state == 'I' || state == 'S') {

			// ... and button 0 is pressed, show the current average
			if(!(BUTTON2_PIN & (1<<BUTTON2))) {
				uart_interrupt_disable();
				display_uint16_time(average());
				my_delay_ms(1500);
				while(!(BUTTON2_PIN & (1<<BUTTON2)));
				uart_interrupt_enable();
			}

		}
	}
}

