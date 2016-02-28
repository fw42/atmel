/***************************************************************
 * (c) 2008 by Florian Weingarten <flo@hackvalue.de>           *
 * Published under the terms of the GNU General Public License *
 ***************************************************************/

#define	DCF77_VCC_DDR	DDRD
#define DCF77_VCC_PORT	PORTD
#define DCF77_VCC	PD7

#define DCF77_PON_DDR	DDRB
#define DCF77_PON_PORT	PORTB
#define DCF77_PON	PB0

void init_dcf77(uint8_t mode);
void enable_dcf77_interrupt(void);
void disable_dcf77_interrupt(void);
void dcf77_power_on(void);
void dcf77_power_off(void);

