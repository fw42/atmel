/***************************************************************
 * (c) 2008 by Florian Weingarten <flo@hackvalue.de>           *
 * Published under the terms of the GNU General Public License *
 ***************************************************************/

#define BAUD 9600UL

#define UBRR_VAL	((F_CPU+BAUD*8)/(BAUD*16)-1)
#define BAUD_REAL	(F_CPU/(16*(UBRR_VAL+1)))
#define BAUD_ERROR	((BAUD_REAL*1000)/BAUD)

#if ((BAUD_ERROR<990) || (BAUD_ERROR>1010))
	#error Baudrate error too high
#endif

void init_uart(void);
void tx_byte(uint8_t);
