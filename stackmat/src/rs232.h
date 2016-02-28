/**************************************************************
 * Selfmade Stackmat Display - (c) 2008 by Florian Weingarten *
 *                                                            *
 * This file is free software! published under the terms of   *
 * GNU general public license (GPL)                           *
 *                                                            *
 * firmware version 0.1 (first working published version)     *
 **************************************************************/

#define BAUD 1200UL

#define UBRR_VAL	((F_CPU+BAUD*8)/(BAUD*16)-1)
#define BAUD_REAL	(F_CPU/(16*(UBRR_VAL+1)))
#define BAUD_ERROR	((BAUD_REAL*1000)/BAUD)

#if ((BAUD_ERROR<990) || (BAUD_ERROR>1010))
	#error Baudrate error too high
#endif

void init_uart(void);
void uart_interrupt_enable(void);
void uart_interrupt_disable(void);

