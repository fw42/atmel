/***************************************************************
 * (c) 2008 by Florian Weingarten <flo@hackvalue.de>           *
 * Published under the terms of the GNU General Public License *
 ***************************************************************/

#ifndef MAX7219_H
#define MAX7219_H

// Register Definitionen
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
#define DATA_SHUTDOWN   0x00
#define DATA_NORMALMODE 0x01
#define DATA_TEST       0x01
#define DATA_NOTEST     0x00

// MAX7219 Kommandos
#define DISPLAY_SHUTDOWN    (   (REG_SHUTDOWN<<8) | (DATA_SHUTDOWN)   )
#define DISPLAY_NORMALMODE  (   (REG_SHUTDOWN<<8) | (DATA_NORMALMODE) )
#define DISPLAY_TEST        ((REG_DISPLAYTEST<<8) | (DATA_TEST)       )
#define DISPLAY_NOTEST      ((REG_DISPLAYTEST<<8) | (DATA_NOTEST)     )

// Prototypen aus max7219.c
void init_max7219(uint8_t);
void max7219_set_register(uint16_t);
void max7219_all_digits_off(void);

// Register byteweise setzen, Adresse und Daten getrennt
#define max7219_set_register_b(address, data) max7219_set_register( ((address)<<8) | (data) )

#endif
