/**************************************************************
 * Selfmade Stackmat Display - (c) 2008 by Florian Weingarten *
 *                                                            *
 * This file is free software! published under the terms of   *
 * GNU general public license (GPL)                           *
 *                                                            *
 * firmware version 0.1 (first working published version)     *
 **************************************************************/

#include <avr/io.h>
#include "rs232.h"

void init_uart(void) {

	// Enable receiver
	UCSR0B = (1<<RXEN0);

	// 8 data bits, 1 stop bit, no parity
	UCSR0C |= (1 << UCSZ00 | 1 << UCSZ01);
	
	// set the upper byte of the baud rate
	UBRR0H = UBRR_VAL >> 8;

	// set the lower byte of the baud rate
	UBRR0L = UBRR_VAL & 0xFF;

}

// enable receive complete interrupt
void uart_interrupt_enable(void) {
	UCSR0B |= (1 << RXCIE0);
}

// disable receive complete interrupt
void uart_interrupt_disable(void) {
	UCSR0B &= ~(1 << RXCIE0);
}

