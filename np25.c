/*

Chris Chung August 2014, simpleavr@gmail.com
nonpariel physical (NP) is an standalone calculator microcode emulator;

Compile w/ (example only, substitute w/ your setup path)

/msp430-gcc -Os -Wall -ffunction-sections -fdata-sections -fno-inline-small-functions -Wl,-Map=np25.map,--cref -Wl,--relax -Wl,--gc-sections -I /cygdrive/c/mspgcc-20120406-p20120502/bin/../msp430/include -mmcu=msp430g2553 -o np25.elf np25.c

September 2014, cc
. this is a preview release upon request, project is not bugs free.

. based on TI msp430g25x3 mcu, 16KB flash, 512B ram
. based on work from Eric Smith's nonpariel
. emualtes woodstock or spice core
. this is a hobby project not suitable for any use.
. please observe same GPL licensing terms as the original Nonpariel package
. notice from orignal Nonpareil package

Copyright 1995, 2003, 2004, 2005 Eric L. Smith <eric@brouhaha.com>

Nonpareil is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.  Note that I am not
granting permission to redistribute or modify Nonpareil under the
terms of any later version of the General Public License.

Nonpareil is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program (in the file "COPYING"); if not, write to the
Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111, USA.
*/

//#define C_SPICE
#define EMBEDDED
#define C_STANDALONE

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef EMBEDDED

#define G2553
#define MHZ	8

#include <msp430.h>
#include "../local/common.h"

#ifdef C_STANDALONE
static volatile uint16_t ticks = 0;
static volatile uint8_t clicks = 1;

/*

               MSP430G25x3
             -----------------    
         /|\|                 |  
          | |                 | 
          --|RST  digit 0 P2.6|--\
            |     digit 1 P1.0|--\       digits bus (12)
            |     digit 2 P2.7|-----//--+---------+
            |     digit 3 P1.1|--/      |         ~  3 units of 4 digits bubble led
            |     digit 4 P1.2|--/      |         |
            |     digit 5 P1.3|--/      |    +---------+ +---------+ +---------+
            |     digit 6 P2.0|--/      |    | % % % % | | % % % % | | % % % % |----+
            |     digit 7 P1.4|--/      ~    +---------+ +---------+ +---------+    |
            |     digit 8 P1.5|--/      |                                           |
            |     digit 9 P2.1|--/      | digits bus x 3 scan lines                 |
            |     digit a P1.6|--/      +-----+-----+---/ ~~ /--+-----+-----+-----+ |
            |     digit b P1.7|--/  _=_ | _=_ | _=_ | _/ ~~ /=_ | _=_ | _=_ | _=_ | |
			|      scan X P2.5|-----o o-+-o o-+-o o-+-/ ~~ /o o-+-o o-+-o o-+-o o-+ |
			|      scan Y P2.4|-----o o-+-o o-+-o o-+/ ~~ /-o o-+-o o-+-o o-+-o o-+ |
			|      scan Z P2.3|-----o o-+-o o-+-o o-/ ~~ /+-o o-+-o o-+-o o-+-o o-+ |
            |                 |                                                     ~
			|                 |      (32 tactile button)                            |
            |                 |                                                     |
			|                 |        +--------+                                   |
			|             P2.2|--+---->|CLK ~CLR|---+---- Vcc                       |
			|                 |  |     |       B|---+                               |
			|                 |  |     |  0(d)QA|-\                                 |
			|                 |  |     |  1(f)QB|-\                segments bus (8) |
			|                 |  -     |  2(b)QC|--------------//-------------------+
			|                 | | |    |  3(a)QD|-/
			|                 | | |1k  |  4(g)QE|-/  74hc164 segment driver
			|                 |  -     |  5(e)QF|-/
			|                 |  |     |  6(c)QG|-/
			|                 |  +---->|A 7(h)QH|-/
			|                 |  |     +--------+
            |                 | === (102)    1 wire shift register via RC delay
            |                 | ===  1nF	 to shift 1, Hi-Low-Hi
            +-----------------+  |           to shift 0, Hi-Looooooow-Hi (>20us Lo)
                                ___
                                ///

               74hc164
               o---------+
               |A     Vcc|
               |B      QH|
               |QA     QG|
               |QB     QF|
               |QC     QE|
               |QD   ~CLR|
               |Gnd   CLK|
               +---------+

	4 digit bubble led from sparkfun
    +--+--+--+--+--+ 
   |a  b  2  f  d  g|
   |1  e  c  3  h  4|
    +--+--+--+--+--+ 

	msp430g25x3 pinout (a=port1, b=port2)
    +--+--+--+--+--+--+--+--+--+ 
   |- b6 b7 CK IO a7 a6 b5 b4 b3|
   |                            |
   |+ a0 a1 a2 a3 a4 a5 b0 b1 b2|
    o--+--+--+--+--+--+--+--+--+ 
*/


#define C_PCB

#ifdef C_PCB

static const uint8_t digit_map_p1[] = {
	0x00, 1<<0, 0x00, 1<<1,
	1<<2, 1<<3, 0x00, 1<<4,
	1<<5, 0x00, 1<<6, 1<<7,
};

static const uint8_t digit_map_p2[] = {
	1<<6, 0x00, 1<<7, 0x00,
	0x00, 0x00, 1<<0, 0x00,
	0x00, 1<<1, 0x00, 0x00,
};

#define CLK_LOW	P2OUT &= ~BIT2
#define CLK_HI	P2OUT |= BIT2

#define SPIN	P2IN
#define SPOUT	P2OUT
#define SPREN	P2REN
#define SPDIR	P2DIR

#define SCANX	BIT5
#define SCANY	BIT4
#define SCANZ	BIT3
#define ALL_SCANS	(SCANX|SCANY|SCANZ)

#define SEG_D	BIT0
#define SEG_F	BIT1
#define SEG_B	BIT2
#define SEG_A	BIT3
#define SEG_G	BIT4
#define SEG_E	BIT5
#define SEG_C	BIT6
#define SEG_H	BIT7

#else

static const uint8_t digit_map_p1[] = {
	1<<0, 1<<7, 1<<1, 1<<2,
	0, 0, 0, 0,
	0, 0, 0, 0,
};

static const uint8_t digit_map_p2[] = {
	0, 0, 0, 0,
	1<<0, 1<<1, 1<<2, 1<<3,
	1<<4, 1<<5, 1<<6, 1<<7,
};

#define CLK_LOW	P1OUT &= ~BIT3
#define CLK_HI	P1OUT |= BIT3

#define SPIN	P1IN
#define SPOUT	P1OUT
#define SPREN	P1REN
#define SPDIR	P1DIR
#define SCANX	BIT4
#define SCANY	BIT5
#define SCANZ	BIT6
#define ALL_SCANS	(SCANX|SCANY|SCANZ)

#define SEG_A	BIT0
#define SEG_B	BIT5
#define SEG_C	BIT2
#define SEG_D	BIT6
#define SEG_E	BIT3
#define SEG_F	BIT4
#define SEG_G	BIT7
#define SEG_H	BIT1

#endif

#define ALL_DIGITS_P1	\
   (digit_map_p1[0]|digit_map_p1[1]|digit_map_p1[2]|digit_map_p1[3]|	\
	digit_map_p1[4]|digit_map_p1[5]|digit_map_p1[6]|digit_map_p1[7]|	\
	digit_map_p1[8]|digit_map_p1[9]|digit_map_p1[10]|digit_map_p1[11])
#define ALL_DIGITS_P2	\
   (digit_map_p2[0]|digit_map_p2[1]|digit_map_p2[2]|digit_map_p2[3]|	\
	digit_map_p2[4]|digit_map_p2[5]|digit_map_p2[6]|digit_map_p2[7]|	\
	digit_map_p2[8]|digit_map_p2[9]|digit_map_p2[10]|digit_map_p2[11])

static const uint8_t seg_map[] = {
   SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F,
   SEG_B|SEG_C,
   SEG_A|SEG_B|SEG_D|SEG_E|SEG_G,
   SEG_A|SEG_B|SEG_C|SEG_D|SEG_G,
   SEG_B|SEG_C|SEG_F|SEG_G,
   SEG_A|SEG_C|SEG_D|SEG_F|SEG_G,
   SEG_A|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G,
   SEG_A|SEG_B|SEG_C,
   SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G,
   SEG_A|SEG_B|SEG_C|SEG_D|SEG_F|SEG_G,
   SEG_E|SEG_G,	// r, H/F, o, p, e, blank
#ifdef C_SPICE
   SEG_B|SEG_C|SEG_E|SEG_F|SEG_G,
#else
   SEG_A|SEG_E|SEG_F|SEG_G,
#endif
   SEG_C|SEG_D|SEG_E|SEG_G,
   SEG_A|SEG_B|SEG_E|SEG_F|SEG_G,
   SEG_A|SEG_D|SEG_E|SEG_F|SEG_G,
   0x00,
};

/*
keys layout
	// 25
	" SST  BST  GTO   F    G ",	// 0xb3, 0xb2, 0xb1, 0xb0, 0xb4
	" X.Y  RV   STO  RCL  E+ ",	// 0x43, 0x42, 0x41, 0x40, 0x44
	" ETR  ETR  CHS  EEX  CLX",	// 0xd3,       0xd1, 0xd0, 0xd4
	"  -     7     8      9  ",	// 0x63, 0x62, 0x61, 0x60
	"  +     4     5      6  ",	// 0x93, 0x92, 0x91, 0x90
	"  x     1     2      3  ",	// 0x73, 0x72, 0x71, 0x70
	"  /     0     .     R/S ", // 0xa3, 0xa2, 0xa1, 0xa0

	// 34c
	"  A    B   GSB   F    G ",	// 0x34, 0x33, 0x32, 0x31, 0x30
	" X.Y  GTO  STO  RCL   h ",	// 0x94, 0x93, 0x92, 0x91, 0x90
	" ETR  ETR  CHS  EEX  CLX",	// 0x73,       0x72, 0x71, 0x70
	"  -     7     8      9  ",	// 0xa3, 0xa2, 0xa1, 0xa0
	"  +     4     5      6  ",	// 0x43, 0x42, 0x41, 0x40
	"  x     1     2      3  ",	// 0xd3, 0xd2, 0xd1, 0xd0
	"  /     0     .     R/S ", // 0x63, 0x62, 0x61, 0x60

raw scancode on pcb version, code is digit * 4 + [123] for [XYZ]

                       [8Z]	            ,35,
   [1X] [3X] [4X] [8X] [8Y]	, 5,13,17,33,34,
   [1Y] [3Y] [4Y] [7Y] [aY] , 6,14,18,30,42,
   [1Z] [3Z] [4Z] [7X] [aX] , 7,15,19,29,41,
   [5Y]  [5Z]   [7Z]   [aZ] ,22,   23,31,43,
   [0Z]  [2Z]   [bZ]   [9Z] , 3,   11,47,39,
   [0X]  [2X]   [bX]   [9X] , 1,    9,45,37,
   [0Y]  [2Y]   [bY]   [9Y] , 2,   10,46,38,
   scan lines X, Y, Z

*/
static const uint8_t key_map[] = {		// 12 digits (no digit 6) x 3 scan lines
#ifdef C_SPICE
	0x00, 0xd3, 0x43, 0x63, 
	0x00, 0x34, 0x94, 0x73,
	0x00, 0xd2, 0x42, 0x62, 
	0x00, 0x33, 0x93, 0x73,	//0x72,
	0x00, 0x32, 0x92, 0x71, 
	0x00, 0x00, 0xa3, 0xa2,
	0x00, 0x00, 0x00, 0x00, 
	0x00, 0x71, 0x91, 0xa1,
	0x00, 0x31, 0x30, 0x00, 
	0x00, 0xd0, 0x40, 0x60,
	0x00, 0x70, 0x90, 0xa0,
	0x00, 0xd1, 0x41, 0x61,
#else
	0x00, 0x73, 0x93, 0xa3, 
	0x00, 0xb3, 0x43, 0xd3,
	0x00, 0x72, 0x92, 0xa2, 
	0x00, 0xb2, 0x42, 0xd3,	//0xd2,
	0x00, 0xb1, 0x41, 0xd1, 
	0x00, 0x00, 0x63, 0x62,
	0x00, 0x00, 0x00, 0x00, 
	0x00, 0xd0, 0x40, 0x61,
	0x00, 0xb0, 0xb4, 0x00, 
	0x00, 0x70, 0x90, 0xa0,
	0x00, 0xd4, 0x44, 0x60,
	0x00, 0x71, 0x91, 0xa1,
#endif
};

#else

#include "../local/uart.h"
#define putchar(x)	uart_putc(x)

#endif

#else

#include <unistd.h>
#include <sys/select.h>
#include <termios.h>

#endif

#ifdef C_STANDALONE
#else 

static const uint8_t key_map[256] = {
	// 25c
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00,	// 0x0f
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 0x1f
	0xd3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0xa3, 0x00, 0x63, 0x91, 0x93,	// 0x2f
	0x92, 0x72, 0x71, 0x70, 0xa2, 0xa1, 0xa0, 0x62, 0x61, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 0x3f
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 0x4f
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 0x5f
	0x00, 0xb3, 0x00, 0xd4, 0xb1, 0x41, 0xb0, 0xb4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 0x6f
	0x00, 0x43, 0x40, 0xb2, 0x44, 0x00, 0x00, 0x42, 0xd0, 0x00, 0xd1, 0x00, 0x00, 0x00, 0x00, 0x00,	// 0x7f
	// 34c
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00,	// 0x0f
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 0x1f
	0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd3, 0x63, 0x00, 0xa3, 0x41, 0x43,	// 0x2f
	0x42, 0xd2, 0xd1, 0xd0, 0x62, 0x61, 0x60, 0xa2, 0xa1, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 0x3f
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 0x4f
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 0x5f
	0x00, 0x34, 0x00, 0x70, 0x32, 0x92, 0x31, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 0x6f
	0x00, 0x94, 0x91, 0x33, 0x90, 0x00, 0x00, 0x93, 0x71, 0x00, 0x72, 0x00, 0x00, 0x00, 0x00, 0x00,	// 0x7f
};

#endif

#ifdef C_SPICE
#include "rom_34c.h"
#else
#include "rom_25.h"
#endif
#include "np25.h"

static volatile uint8_t _pgm_run=1;
static volatile uint8_t _state=0;
static volatile uint8_t _key=0;

#define ST_RAM_LOADED 		BIT0
#define ST_KEY_PRESSED		BIT1
#define ST_KEY_RELEASED		BIT2
#define ST_HW_SLOW			BIT6
#define ST_HW_TEST			BIT7

/*
  base=142
  64+64+64
  4 blocks 112+112+112+112=448
  142 + [224] = 366 used + 32 ram_reserve = 392
  > offset 224
  get block 2+3 (224) write 192 to info BCD, 32 into ram_reserve
*/

//________________________________________________________________________________
void flash_write(uint16_t ptr, char *src, uint8_t cnt) {
	volatile char *flash = (char*) ptr;
	FCTL2 = FWKEY+FSSEL0+FN1;
	FCTL1 = FWKEY+ERASE;
	FCTL3 = FWKEY;
	*flash = 0;

	FCTL1 = FWKEY+WRT; 
	uint8_t i;
	for (i=0;i<cnt;i++) *flash++ = *src++;
	FCTL1 = FWKEY;
	FCTL3 = FWKEY+LOCK; 
}

//________________________________________________________________________________
int main() {

#ifdef EMBEDDED
    WDTCTL  = WDTPW + WDTHOLD; 	// Stop WDT
	__use_cal_clk(MHZ);
	FCTL2 = FWKEY + FSSEL0 + FN1;	// MCLK/3 for Flash Timing Generator

#ifdef C_STANDALONE

	TA0CCR0  = MHZ * 1000;		// start w/ 1ms
	TA0CCTL0 = CCIE;
	TA0CTL = TASSEL_2 + MC_2;	// SMCLK, cont.

	P1SEL = P2SEL = 0;

	P1OUT = ALL_DIGITS_P1;
	P2OUT = ALL_DIGITS_P2;
	P1DIR = P2DIR = 0xff;

	SPOUT |= ALL_SCANS;
	SPREN |= ALL_SCANS;
	SPDIR &= ~ALL_SCANS;

#else
	uint8_t usage = 0;
	uart_init();
#endif

    __bis_SR_register(GIE);

#else
	set_conio_terminal_mode();

#endif

	woodstock_new_processor();
 
	/*
	key_map['0'] = 0x92; key_map['1'] = 0x72; key_map['2'] = 0x71; key_map['3'] = 0x70;
	key_map['4'] = 0xa2; key_map['5'] = 0xa1; key_map['6'] = 0xa0; key_map['7'] = 0x62;
	key_map['8'] = 0x61; key_map['9'] = 0x60; key_map['.'] = 0x91; key_map['-'] = 0x63;
	key_map['+'] = 0xa3; key_map['*'] = 0x73; key_map['/'] = 0x93; key_map['\r'] = 0x90;	// run/stop
	key_map['a'] = 0xb3; key_map['s'] = 0xb2; key_map['d'] = 0xb1; key_map['f'] = 0xb0; key_map['g'] = 0xb4; // SST BST GTO F   G
	key_map['q'] = 0x43; key_map['w'] = 0x42; key_map['e'] = 0x41; key_map['r'] = 0x40; key_map['t'] = 0x44; // X.Y RV  STO RCL E+
	key_map[' '] = 0xd3; key_map['z'] = 0xd1; key_map['x'] = 0xd0; key_map['c'] = 0xd4;					     // ETR ETR CHS EEX CLX
	*/

	while (clicks) __asm("nop");
	switch (_key) {		// hold + power_on enable session options
		case 33:	// F key, h/w test
			_state |= ST_HW_TEST;
			break;
		case 34:	// G key, slow CPU
			_state |= ST_HW_SLOW;
			break;
		default:
			break;
	}//switch
	while (_key) __asm("nop");
	_state &= ~(ST_RAM_LOADED|ST_KEY_PRESSED|ST_KEY_RELEASED);

	uint8_t done=0;
	uint8_t c=0;
	uint8_t release_in=0;


	woodstock_set_ext_flag (3, _pgm_run);		// set run mode

	while (!done) {
#ifdef C_STANDALONE
		if (_state) {
			if (_state & ST_KEY_PRESSED) {
				if (!(_state&ST_RAM_LOADED)) {
					//______ lets load ram from flash (pretend continous memory) at 1st key
					_state |= ST_RAM_LOADED;
					c = 7*__USE_RAM;
					if (((char*) act_reg->ram)[c] != 0xff)
						while (c--) ((char*) act_reg->ram)[c] = *((char*) (0xfc00+c));
				}//if
				if (_key == 35) {
					woodstock_set_ext_flag (3, _pgm_run ^= 1);		// pgm-run toggle
					//___ to run mode, write flash
					if (_pgm_run) flash_write(0xfc00, (char*)act_reg->ram, 7*__USE_RAM);
				}//if
				else {
					if (key_map[_key]) woodstock_press_key(key_map[_key]);
				}//else
				_state &= ~ST_KEY_PRESSED;
			}//if
			else if (_state & ST_KEY_RELEASED) {
				woodstock_release_key();
				_state &= ~ST_KEY_RELEASED;
			}//if
		}//if
#else
#ifdef EMBEDDED
		if (c || uart_getc(&c)) {
			//P1OUT ^= BIT0;
			if (!usage) {
				usage = 1;
				uart_puts("\x1b[1;1H");
				uart_puts("\x1b[0J");
				uart_puts("\n\r\n\r\n\r");
				uart_puts(" SST  BST  GTO   F    G \n\r");
				uart_puts(" X.Y  RV   STO  RCL  E+ \n\r");
				uart_puts(" ETR  ETR  CHS  EEX  CLX\n\r");
				uart_puts("  -     7      8      9  \n\r");
				uart_puts("  +     4      5      6  \n\r");
				uart_puts("  x     1      2      4  \n\r");
				uart_puts("  /     0      .     R/S \n\r\n\r");
				uart_puts("\x1b[9A");
			}//if
#else
		if (kbhit()) {
			c = getch();
#endif
			switch (c) {
				case 27: 
					done = 1; 
					break;
				case '\\': 
					woodstock_new_processor();
					_pgm_run = 0;
					break;
				case '=': 
					woodstock_set_ext_flag (3, _pgm_run ^= 1);		// pgm-run toggle
					//if (_pgm_run) flash_write(0xfc00, (char*)act_reg->ram, 7*__USE_RAM);
					break;
				default:
#ifdef C_SPICE
					c += 128;
#endif
					if (key_map[c]) {
						woodstock_press_key(key_map[c]);
						release_in = 10;
					}//if
					break;
			}//switch
			c = 0;
		}//if
#endif
		if (!woodstock_execute_instruction()) break;
		if (release_in) {
			if (release_in == 1) woodstock_release_key();
			release_in--;
		}//if

#ifdef EMBEDDED
		if (_state&ST_HW_SLOW) __delay_cycles(1250);
#else
		//usleep(100);
#endif
	}//while

	return 0;

}

#ifdef EMBEDDED
#ifdef C_STANDALONE

volatile uint16_t _cnt=0;
volatile uint8_t _data=0;
volatile uint8_t _digit=0;
volatile uint8_t _keyscan=0;

uint8_t hwtest_load_segments(uint8_t digit) {
	static uint16_t _val = 0;
	uint8_t res = 0;

	if (digit == 0) _val = _cnt;
	if (digit > 3 && digit < 8) {
		res = seg_map[_val%10];
		_val /= 10;
	}//if
	if (digit == 10) res = seg_map[_key/10];
	if (digit == 11) res = seg_map[_key%10];
	return res;
}
//________________________________________________________________________________
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer0_A0_iSR(void) {

	P1OUT |= ALL_DIGITS_P1;	 // 74hc164 friedly, all digit off
	P2OUT |= ALL_DIGITS_P2;

	uint8_t clock=8,segs=8;
	while (clock--) {
		if ((_data&0x01)) {
			CLK_LOW;
			CLK_HI;
		}//if
		else {
			CLK_LOW;
			__delay_cycles(6);
			segs--;
			CLK_HI;
		}//else
		_data >>= 1;
		//__delay_cycles(25);
	}//while

	if (_digit == 12) {		// key scanning cycle
		if (_key) {
			//_____ check for key release
			uint8_t i;
			i = _key>>2;
			P2OUT &= ~digit_map_p2[i];
			P1OUT &= ~digit_map_p1[i];
			if ((SPIN&ALL_SCANS) == ALL_SCANS) {
				if (_key != 35) _state |= ST_KEY_RELEASED;
				_key = 0;
			}//if
		}//if
		else {
			uint8_t i;

			for (i=0;i<12;!_key&&i++) {
				if (i!=6) {
					//P2OUT |= ALL_DIGITS_P2;
					//P1OUT |= ALL_DIGITS_P1;
					P2OUT &= ~digit_map_p2[i];
					P1OUT &= ~digit_map_p1[i];

					if ((SPIN&ALL_SCANS) != ALL_SCANS) {
						if (!(SPIN&SCANX)) _key = 1+(i<<2);
						if (!(SPIN&SCANY)) _key = 2+(i<<2);
						if (!(SPIN&SCANZ)) _key = 3+(i<<2);
						// SCAN XYZ BIT 5,4,3
						if (_key) {
							_state |= ST_KEY_PRESSED;
							//_key = key;
							break;
						}//if
					}//if
					P2OUT |= digit_map_p2[i];
					P1OUT |= digit_map_p1[i];
				}//if
			}//for
		}//else
	}//if
	else {
		//______ done shifting segments, turn on digit if register says so
		//
		//
		if (act_reg->flags & F_DISPLAY_ON) {
			P1OUT &= ~digit_map_p1[_digit];
			P2OUT &= ~digit_map_p2[_digit];
		}//if
		//
		//
		//TA0CCR0 += MHZ * 1000;	// time till next, 1ms per digit
		// load balance led brightness
		segs += 4;	// 7..15
		TA0CCR0 += MHZ * segs * 60;
	}//else

	//______ prepare for next digit
	if (_digit>=12) {
		_digit = 0;
		TA0CCR0 += MHZ * 50;	// for quick turn-around
	}//if
	else {
		_digit++;
	}//else

	// less frequent key scanning for led brightness
	if (_digit==12 && _keyscan++&0x03) _digit = 0;

	//if (_key && !digit) _data = seg_map[_key/3];
	//if (_key) _data = seg_map[0xe];

	if (_digit == 12) 
		_data = 0;		// key scan cycle, blank all segments
	else
		_data = _state&ST_HW_TEST ? hwtest_load_segments(_digit) : sim_load_segments(_digit);
		
	//
	ticks++;
	if (!(ticks%100)) {
		if (clicks) clicks--;
		_cnt++;
	}//if
	//
}

#else

//________________________________________________________________________________
#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR (void) {
	uart_port1_isr();
}

//_______________________________________________________________________________
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer0_A0_ISR (void) {
	uart_timera0_isr();
}

//________________________________________________________________________________
#pragma vector=WDT_VECTOR
__interrupt void WDT_ISR (void) {
	//ticks++;
}
#endif

#endif

