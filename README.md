

NP Calculator Emulator
======================
Nonpariel Physical (NP) is an standalone calculator microcode emulator

Features
_________________
* selectable rom from HP-21, HP-25C and HP-33C.
* press-n-hold + switch on to select one of the three ROMSs.
* a setup menu to set the default ROM to use, plus ability to set speed (fast / slow).
* both 25C and 33C programs are stored separately.
* a greetings message set and show, handy as a gift item

Description
___________

Compile w/ (example only, substitute w/ your setup path)

/msp430-gcc -Os -Wall -ffunction-sections -fdata-sections -fno-inline-small-functions -Wl,-Map=np25.map,--cref -Wl,--relax -Wl,--gc-sections -I /cygdrive/c/mspgcc-20120406-p20120502/bin/../msp430/include -mmcu=msp430g2553 -o np25.elf np25.c

October 2014, cc
* major code clean-up
* includes power-on switchable roms for 21, 25C and 33C
* allow defaults (rom and speed) be saved
* add greeting message
* rom_34c.h, rom for 34C is included for further development only, not emulated at this version

September 2014, cc
* this is a preview release upon request, project is not bugs free.
* based on TI msp430g25x3 mcu, 16KB flash, 512B ram
* based on work from Eric Smith's nonpariel
* emualtes woodstock or spice core
* this is a hobby project not suitable for any use.
* please observe same GPL licensing terms as the original Nonpariel package
* notice from orignal Nonpareil package

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


Parts list
__________

* msp430g2553 (or other G series dip 20pin devices w/ 16k flash)
* 2x sparkfun 4 digit bubble led module
* 32x tactile buttons
* 1x CR2032 SMD battery holder
* 1x 47k, 1x 1k resistors
* 2x 100nF (104), 1x 1nF (102) capacitor

Application Notes
_________________

* press-n-hold 1st row, 5th column (from top) for hp33C
* press-n-hold 2nd row, 5th column for hp25C
* press-n-hold 3nd row, 5th column for hp21
* press-n-hold 4th row, 4th column for version info
* press-n-hold 1st row, 4th column to show greetings
* press-n-hold 1st row, 3rd column to edit greetings, use 0-9 to enter numbers and letters, Run/Stop to advance digit, Cls to abandon entry
* press-n-hold Pgm/Run toggle button to edit default startup settings, use 'F' key to select rom, 'G' key to select speed, Pgm/Run will save setting and exit setup


Schematic
_________


    * runs off 3V button cell

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


Additional Notes
________________





