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
#define MHZ	12

#include <msp430.h>
#include "../local/common.h"

#ifdef C_STANDALONE
static volatile uint16_t _ticks = 0;
static volatile uint8_t _clicks = 1;

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

static const 
//__attribute__ ((section (".data_fe"))) 
uint8_t digit_map_p1[] = {
	0x00, 1<<0, 0x00, 1<<1,
	1<<2, 1<<3, 0x00, 1<<4,
	1<<5, 0x00, 1<<6, 1<<7,
};

static const 
//__attribute__ ((section (".data_fe"))) 
const uint8_t digit_map_p2[] = {
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

static const 
//__attribute__ ((section (".data_fe"))) 
uint8_t seg_map[] = {
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
//#ifdef C_SPICE
//#else
   SEG_A|SEG_E|SEG_F|SEG_G, 			
//#endif
   SEG_C|SEG_D|SEG_E|SEG_G,
   SEG_A|SEG_B|SEG_E|SEG_F|SEG_G,
   SEG_A|SEG_D|SEG_E|SEG_F|SEG_G,
   0x00,
   SEG_B|SEG_C|SEG_E|SEG_F|SEG_G, 		// for spice as 0xb
   //________________ a to z
   SEG_A|SEG_B|SEG_C|SEG_E|SEG_F|SEG_G,	// A
   SEG_C|SEG_D|SEG_E|SEG_F|SEG_G,		// b
   SEG_D|SEG_E|SEG_G,					// c
   SEG_B|SEG_C|SEG_D|SEG_E|SEG_G,		// d
   SEG_A|SEG_D|SEG_E|SEG_F|SEG_G,		// E
   SEG_A|SEG_E|SEG_F|SEG_G,				// F
   SEG_A|SEG_C|SEG_D|SEG_E|SEG_F,		// G
   SEG_C|SEG_E|SEG_F|SEG_G,				// h
   SEG_E|SEG_F,							// i
   SEG_B|SEG_C|SEG_D,					// j
   SEG_D|SEG_F|SEG_G,					// k
   SEG_D|SEG_E|SEG_F,					// L
   SEG_A|SEG_C|SEG_E,					// m
   SEG_C|SEG_E|SEG_G,					// n
   SEG_C|SEG_D|SEG_E|SEG_G,				// o
   SEG_A|SEG_B|SEG_E|SEG_F|SEG_G,		// P
   SEG_A|SEG_B|SEG_C|SEG_F|SEG_G,		// q
   SEG_E|SEG_G,							// r
   SEG_A|SEG_C|SEG_D|SEG_F|SEG_G,		// S
   SEG_D|SEG_E|SEG_F|SEG_G,				// t
   SEG_C|SEG_D|SEG_E,					// u
   SEG_C|SEG_D|SEG_E|SEG_F,				// v
   SEG_B|SEG_D|SEG_F,					// w
   SEG_B|SEG_C|SEG_E|SEG_F|SEG_G,		// X
   SEG_B|SEG_C|SEG_D|SEG_F|SEG_G,		// y
   SEG_A|SEG_B|SEG_D|SEG_E|SEG_G,		// Z
   SEG_A|SEG_D|SEG_E|SEG_F|SEG_G,		// [
   SEG_A|SEG_B|SEG_F,					// ^
   SEG_A|SEG_B|SEG_C|SEG_D|SEG_G,		// ]
   SEG_C|SEG_F|SEG_G,					// blackslash
   SEG_D,								// _
   SEG_B,								// '
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

#define __USE_RAM	32	// our highest model 33c has 32 units

#include "rom_21.h"
#include "rom_25.h"
#include "rom_33c.h"
#include "np25.h"

static volatile uint8_t _pgm_run=1;
static volatile uint8_t _state=0;
static volatile uint8_t _key=0;
static int8_t _greetings[12];

#define ST_RAM_LOADED 		BIT7
#define ST_KEY_PRESSED		BIT6
#define ST_KEY_RELEASED		BIT5
#define ST_ALPHA_MSG		BIT4
#define ST_HW_SLOW			BIT3
#define ST_HW_TEST			BIT2
#define ST_ROM_BIT1			BIT1
#define ST_ROM_BIT0  		BIT0
#define ST_ROM				(ST_ROM_BIT1|ST_ROM_BIT0)

/*
  base=142
  64+64+64
  4 blocks 112+112+112+112=448
  142 + [224] = 366 used + 32 ram_reserve = 392
  > offset 224
  get block 2+3 (224) write 192 to info BCD, 32 into ram_reserve
*/

//________________________________________________________________________________
void flash_write(uint8_t bank, char *src, uint8_t cnt) {
	char *flash = (char*) 0x1040;
	if (bank) flash += 0x40;
	FCTL2 = FWKEY+FSSEL0+FN1;
	FCTL1 = FWKEY+ERASE;
	FCTL3 = FWKEY;
	*flash = 0;

	FCTL1 = FWKEY+WRT; 

	uint8_t i;
	for (i=0;i<cnt;i++) *flash++ = *src++;
	*flash++ = _state;
	if (!bank)
		for (i=0;i<12;i++) *flash++ = _greetings[i];

	FCTL1 = FWKEY;
	FCTL3 = FWKEY+LOCK; 
}

#define _MSG_POS	act_reg->f
#define _MSG_LEN	act_reg->p
#define _EDIT_POS 	act_reg->key_buf
#define _MSG_PTR	act_reg->rom
#define _LAST_KEY	act_reg->flags

//volatile uint8_t _MSG_POS=0, _MSG_LEN=0;
//volatile const char *_MSG_PTR=0;

/*
hp-25 5
pos 11 digit 0(x).......     11(h)
pos 10 digit 0(x).......10(h)11(p)
pos 2 digit 0(x)...9(h)10(p)11(-)
*/


//________________________________________________________________________________
uint8_t char_at_digit(uint8_t digit) {
	uint8_t seg=0;
	if ((digit==_EDIT_POS) && (_ticks&0x200)) return 0;
	if (digit>=_MSG_POS) {
		uint8_t i = digit-_MSG_POS;
		if (i < _MSG_LEN) {
			if (_MSG_PTR[i] >= '0') { if (i >= 'A')
					seg = seg_map[_MSG_PTR[i] - 32 - '0'];
				else
					seg = seg_map[_MSG_PTR[i] - '0'];
			}//if
		}//if
	}//if
	return seg;
}
//________________________________________________________________________________
void show_msg(const uint8_t *msg, uint8_t delay_units) {

	_MSG_PTR = msg;
	while (*msg++) _MSG_LEN++;
	_MSG_POS = 11;
	_state |= ST_ALPHA_MSG;
	//______ accelerate.....brake 6,5,4,3,2,1,2,3,4,5,6
	//                            b,a,9,8,7,6,5,4,3,2,1
	while (_MSG_POS--) {
		_clicks = _MSG_POS>=6 ? _MSG_POS-5 : 6-_MSG_POS;
		LPM0;
	}//while
	_MSG_POS = 0;

	if (delay_units) _clicks = delay_units;
	LPM0;
	_state &= ~ST_ALPHA_MSG;
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

 
	/*
	key_map['0'] = 0x92; key_map['1'] = 0x72; key_map['2'] = 0x71; key_map['3'] = 0x70;
	key_map['4'] = 0xa2; key_map['5'] = 0xa1; key_map['6'] = 0xa0; key_map['7'] = 0x62;
	key_map['8'] = 0x61; key_map['9'] = 0x60; key_map['.'] = 0x91; key_map['-'] = 0x63;
	key_map['+'] = 0xa3; key_map['*'] = 0x73; key_map['/'] = 0x93; key_map['\r'] = 0x90;	// run/stop
	key_map['a'] = 0xb3; key_map['s'] = 0xb2; key_map['d'] = 0xb1; key_map['f'] = 0xb0; key_map['g'] = 0xb4; // SST BST GTO F   G
	key_map['q'] = 0x43; key_map['w'] = 0x42; key_map['e'] = 0x41; key_map['r'] = 0x40; key_map['t'] = 0x44; // X.Y RV  STO RCL E+
	key_map[' '] = 0xd3; key_map['z'] = 0xd1; key_map['x'] = 0xd0; key_map['c'] = 0xd4;					     // ETR ETR CHS EEX CLX
	*/
#define RAM_OFFSET	(7*9)
#define RAM_SIZE	(7*7)		// 49 program steps

	woodstock_clear_memory();
	//______ load from flash, we just need the status config, but we load everything since
	//       we might need to write it back and flash always write in full blocks
	uint8_t idx = RAM_SIZE + 1 + 12;	// 49 program steps + config byte + 12 byte greeting
	while (idx--) ((char*) act_reg->ram)[idx+RAM_OFFSET] = *((char*) (0x1040+idx));
	_state = ((char*) (act_reg->ram))[RAM_OFFSET+RAM_SIZE] & (ST_HW_SLOW|ST_ROM);

	for (idx=0;idx<12;idx++) _greetings[idx] = ((uint8_t*) act_reg->ram)[RAM_OFFSET+50+idx];
	_MSG_PTR = (uint8_t*) _greetings;

	LPM0;

	while (_key == 35) _state |= ST_HW_TEST;		// pgm key pressed, enter setup

	_EDIT_POS = 99;
	uint8_t save_state = _state;
	switch (_key) {
		//
		case 17: 	// "goto" key for edit easter egg message
			while (!(_state&ST_KEY_RELEASED)) __asm("nop");
			_state &= ~ST_KEY_RELEASED;
			{
				uint8_t *p= (uint8_t*) _MSG_PTR;
				//uint8_t i=0;
				_EDIT_POS = 0;
				_MSG_POS = 0;
				_MSG_LEN = 12;
				_LAST_KEY = 99;
				_state |= ST_ALPHA_MSG;		// start show
				while (1) {
					LPM0;					// wait key
					uint8_t k = _key;
					if (k == 41) break;
					if (k == 38) {
						_EDIT_POS++;
						p++;
						_LAST_KEY = 99;
						if (_EDIT_POS>=12) {
							//__________ the last 12 bytes are the greetings
							flash_write(0, (char*) act_reg->ram + RAM_OFFSET, RAM_SIZE);
							break;
						}//if
					}//if
					switch (k) {
						case 10: k = 0; break;
						case 9:  k = 1; break;
						case 45: k = 2; break;
						case 37: k = 3; break;
						case 11: k = 4; break;
						case 47: k = 5; break;
						case 39: k = 6; break;
						case 23: k = 7; break;
						case 31: k = 8; break;
						case 43: k = 9; break;
						default: k = 10; break;
					}//switch
					const uint8_t key_adv[] = { 'A', 'D', 'G', 'J', 'M', 'P', 'T', 'W', '[', };
					if (k < 10) {
						if (k == _LAST_KEY) {
							if (k >= 2) {
								if (*p <= '9') {
									*p = key_adv[k-2];
								}//if
								else {
									*p = *p + 1;
									if (*p == key_adv[k-1])
										*p = '0' + k;
								}//else
							}//if
							else {
								if (k == 0) {
									*p = *p == '0' ? '?' : '0';
								}//if
							}//else
						}//if
						else {
							if (k < 10) {
								*p = '0' + k;
								_LAST_KEY = k;
							}//if
						}//else
					}//if
				}//while
			}
			_state &= ~ST_ALPHA_MSG;
			break;
		//
		case 33: 	// F key, show secret message
			show_msg(_MSG_PTR, 100);
			break;
		// right top, going down on same column
		case 34: 
			_state = 1;
			show_msg((const uint8_t*) "HP33C ROM", 100);
			break;
		case 42: 
			_state = 2;
			show_msg((const uint8_t*) "HP25C ROM", 100);
			break;
		case 41: 
			_state = 3;
			show_msg((const uint8_t*) "HP21 ROM", 100);
			break;
		case 43:
			show_msg((const uint8_t*) "NP25 VER 2", 100);
			break;
	}//switch
	if (save_state&ST_HW_SLOW) _state |= ST_HW_SLOW;

	while (_state&ST_HW_TEST) {
		LPM0;
		switch (_key) {
			case 35: 	// pgm/run key
				_state &= ~ST_HW_TEST; 
				flash_write(0, (char*) act_reg->ram + RAM_OFFSET, RAM_SIZE);
				break;
			case 33:	// F key to rotate rom
			// 00..01, 01..10, 10..11, 11..01
				if ((_state&ST_ROM)==0x03)
					_state &= ~ST_ROM;
				_state++;
				break;
			case 34:	// G key to toggle fast and slow cpu
				_state ^= ST_HW_SLOW;
				break;
		}//switch
	}//while

	/*
	if ((_state&ST_ROM)==0x02) {	// 25c use alternate rom
		uint8_t i = 7*16 + 1;
		while (i--) ((char*) act_reg->ram)[i] = *((char*) (0x1040+i));
	}//if
	*/

	woodstock_set_rom(_state&ST_ROM);
	woodstock_new_processor();

	_state &= ~(ST_RAM_LOADED|ST_KEY_PRESSED|ST_KEY_RELEASED);

	uint8_t done=0;
#ifdef C_STANDALONE
#else
	uint8_t c=0;
#endif
	uint8_t release_in=0;

	woodstock_set_ext_flag (3, _pgm_run);		// set run mode

	while (!done) {
#ifdef C_STANDALONE
		if (_state) {
			if (_state & ST_KEY_PRESSED) {
				if (!(_state&ST_RAM_LOADED)) {
					//______ lets load ram from flash (pretend continous memory) at 1st key
					//       need to do it here after cpu initialized
					if (_pgm_run && ((_state&ST_ROM) != 0x03)) {
						_state |= ST_RAM_LOADED;
						char *src = (char*) 0x1040;
						uint8_t c = RAM_SIZE;
						if (!(_state&ST_ROM_BIT0)) src += 0x40;		// next infomem block for hp25
						while (c--) ((char*) act_reg->ram)[c+RAM_OFFSET] = *(src + c);
					}//if
				}//if
				if (_key == 35) {
					woodstock_set_ext_flag (3, _pgm_run ^= 1);		// pgm-run toggle
					//___ to run mode, write flash
					if (_pgm_run && ((_state&ST_ROM) != 0x03)) {
						uint16_t des = 0x1040;
						if (!(_state&ST_ROM_BIT0)) des += 0x40;
						flash_write(_state&ST_ROM_BIT0 ? 0 : 1,
							(char*) act_reg->ram + RAM_OFFSET, RAM_SIZE);
					}//if
				}//if
				else {
					//if (session_key_map[_key]) woodstock_press_key(session_key_map[_key]);
					sim_check_key(_key);
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
		if (_state&ST_HW_SLOW) __delay_cycles(MHZ*150);
#else
		//usleep(100);
#endif
	}//while

	return 0;

}

#ifdef EMBEDDED
#ifdef C_STANDALONE

volatile uint8_t _data=0;
volatile uint8_t _digit=0;
volatile uint8_t _keyscan=0;

//________________________________________________________________________________
uint8_t hwtest_load_segments(uint8_t digit) {
	uint8_t res = 0x0f;

	switch (digit) {
		case 0: res = 'H'-'0'; break;
		case 1: res = 'P'-'0'; break;
		case 2: res = _state&ST_ROM_BIT1 ? 2 : 3; break;
		case 3:
			res = _state&ST_ROM_BIT0 ? (_state&ST_ROM_BIT1 ? 1 : 3) : 5; 
			break; 
		case 5: res = _state&ST_HW_SLOW ? 'S'-'0' : 'F'-'0'; break; 
		case 6: res = _state&ST_HW_SLOW ? 'L'-'0' : 'A'-'0'; break; 
		case 7: res = _state&ST_HW_SLOW ? 'O'-'0' : 'S'-'0'; break; 
		case 8: res = _state&ST_HW_SLOW ? 'W'-'0' : 'T'-'0'; break; 
		//case 7: res = _clicks/10; break;
		//case 8: res = _clicks%10; break;
		case 10: res = _key/10; break;
		case 11: res = _key%10; break;
	}//switch

	return seg_map[res];
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
			//__delay_cycles((MHZ*3)/4);
			__delay_cycles(MHZ);
			segs--;
			CLK_HI;
		}//else
		_data >>= 1;
		//__delay_cycles(25);
	}//while

	if (_digit == 12) {		// key scanning cycle
		uint8_t i;
		if (_key) {
			//_____ check for key release
			i = _key>>2;
			P2OUT &= ~digit_map_p2[i];
			P1OUT &= ~digit_map_p1[i];
			if ((SPIN&ALL_SCANS) == ALL_SCANS) {
				if (_key != 35) _state |= ST_KEY_RELEASED;
				_key = 0;
			}//if
			__delay_cycles(MHZ*100);
		}//if
		else {
			//for (i=0;i<12;!_key&&i++) {
			for (i=0;i<12;i++) {
				if (i!=6) {
					//P2OUT |= ALL_DIGITS_P2;
					//P1OUT |= ALL_DIGITS_P1;
					P2OUT &= ~digit_map_p2[i];
					P1OUT &= ~digit_map_p1[i];

					if ((SPIN&ALL_SCANS) != ALL_SCANS) {
						if (!(SPIN&SCANY)) _key = 2+(i<<2);
						else if (!(SPIN&SCANZ)) _key = 3+(i<<2);
						else if (!(SPIN&SCANX)) _key = 1+(i<<2);
						// SCAN XYZ BIT 5,4,3
						if (_key) {
							_state |= ST_KEY_PRESSED;
							LPM0_EXIT;
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
		if (act_reg->flags&F_DISPLAY_ON || _state&(ST_ALPHA_MSG|ST_HW_TEST)) {
			P1OUT &= ~digit_map_p1[_digit];
			P2OUT &= ~digit_map_p2[_digit];
		}//if
		//
		//
		//TA0CCR0 += MHZ * 1000;	// time till next, 1ms per digit
		// load balance led brightness
		segs += 6;	// 7..15
		TA0CCR0 += MHZ * segs * 60;
	}//else

	//______ prepare for next digit
	if (_digit>=12) {
		_digit = 0;
		TA0CCR0 += MHZ * 50;	// for quick turn-around
	}//if
	else {
		_digit++;
		// less frequent key scanning for led brightness
		if (_digit==12 && _keyscan++&0x03) _digit = 0;
	}//else


	//if (_key && !digit) _data = seg_map[_key/3];
	//if (_key) _data = seg_map[0xe];

	if (_digit == 12) {
		_data = 0;		// key scan cycle, blank all segments
	}//if
	else {
		if (_state&ST_ALPHA_MSG) {
			//_data = _alpha_msg[_digit];
			_data = char_at_digit(_digit);
		}//if
		else {
			if (_state&ST_HW_TEST)
				_data = hwtest_load_segments(_digit);
			else
				_data = sim_load_segments(_digit);
		}//else
	}//else
		
	//
	_ticks++;
	if (!(_ticks%50) && _clicks) {
		_clicks--;
		if (!_clicks) LPM0_EXIT;
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
	//_ticks++;
}
#endif

#endif

