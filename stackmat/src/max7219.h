/**************************************************************
 * Selfmade Stackmat Display - (c) 2008 by Florian Weingarten *
 *                                                            *
 * This file is free software! published under the terms of   *
 * GNU general public license (GPL)                           *
 *                                                            *
 * firmware version 0.1 (first working published version)     *
 **************************************************************/

#include <avr/io.h>

// make sure we only include this file once
#ifndef MAX7219_H
#define MAX7219_H

// register definitions
#define REG_NOP         0x00
#define REG_DIG0        0x01
#define REG_DIG1        0x02
#define REG_DIG2        0x03
#define REG_DIG3        0x04
#define REG_DIG4        0x05
#define REG_DIG5        0x06
#define REG_DIG6        0x07
#define REG_DIG7        0x08
#define REG_DECODE      0x09
#define REG_INTENSITY   0x0A
#define REG_SCANLIMIT   0x0B
#define REG_SHUTDOWN    0x0C
#define REG_DISPLAYTEST 0x0F

// data definitions
#define DATA_SHUTDOWN   0x00
#define DATA_NORMALMODE 0x01
#define DATA_TEST       0x01
#define DATA_NOTEST     0x00

// max7219 commands
#define DISPLAY_SHUTDOWN    ((REG_SHUTDOWN<<8) | (DATA_SHUTDOWN))
#define DISPLAY_NORMALMODE  ((REG_SHUTDOWN<<8) | (DATA_NORMALMODE))
#define DISPLAY_TEST        ((REG_DISPLAYTEST<<8) | (DATA_TEST))
#define DISPLAY_NOTEST      ((REG_DISPLAYTEST<<8) | (DATA_NOTEST))

// prototypes from max7219.c
void init_display(uint8_t);
void set_register(uint16_t);

// set register bytewise
#define set_register_b(address, data) set_register( ((address)<<8) | (data) )

#endif /* MAX7219_H */
