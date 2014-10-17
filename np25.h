/*

Chris Chung August 2014
nonpariel physical (NP) is an standalone calculator microcode emulator;

$Id: proc_woodstock.h 686 2005-05-26 09:06:45Z eric $
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


#define WSIZE 14
#define EXPSIZE 3  // two exponent and one exponent sign digit

typedef uint8_t digit_t;
typedef digit_t reg_t [WSIZE];
typedef digit_t creg_t [WSIZE/2];

#ifdef C_SPICE

#define C_LEFT_SCAN		(WSIZE-2)
#define C_RIGHT_SCAN	3

#else

#define C_LEFT_SCAN 	(WSIZE-1)
#define C_RIGHT_SCAN	2

#endif


#define SSIZE 16
#define STACK_SIZE 2

#define EXT_FLAG_SIZE 16

#define PAGE_SIZE 1024
#define MAX_PAGE 4
#define MAX_BANK 2

typedef uint16_t rom_addr_t;


typedef enum {
	norm,
	branch,
	selftest
} inst_state_t;


#define F_DECIMAL		(1<<0)
#define F_CARRY			(1<<1)
#define F_PREV_CARRY	(1<<2)
#define F_DEL_ROM		(1<<3)
#define F_KEY			(1<<4)
#define F_DISPLAY_ON	(1<<5)
#define F_DISPLAY_14	(1<<6)
#define F_BANK			(1<<7)

typedef struct {
  reg_t a;
  reg_t b;
  reg_t c;
  creg_t y;
  creg_t z;
  creg_t t;
  creg_t m1;
  creg_t m2;

  digit_t f;
  digit_t p;

  uint8_t flags;
  /*
  bool decimal;
  bool carry, prev_carry;
  bool del_rom_flag;
  bool key_flag;      // true if a key is down
  bool display_enable;
  bool display_14_digit;  // true after RESET TWF instruction
  */

  //bool s [SSIZE];                 // ACT flags (status bits)
  //bool ext_flag [EXT_FLAG_SIZE];  // external flags, cause status or CRC bits to get set
  uint16_t ext_flag;  // external flags, cause status or CRC
  uint16_t s;  // ACT flags (status bits)

  uint16_t pc;

  uint16_t del_rom;

  inst_state_t inst_state;

  int16_t sp;  /* stack pointer */
  int16_t stack [STACK_SIZE];

  int16_t prev_pc;  /* used to store complete five-digit octal address of instruction */
  //int crc;
  // keyboard

  uint8_t key_buf;        /* most recently pressed key */

  // display

  //uint8_t left_scan;
  //uint8_t right_scan;
#ifdef C_STANDALONE
#else
  uint8_t display_scan_position;   /* word index, left_scan down to right_scan */
  uint8_t display_digit_position;  /* character index, 0 to MAX_DIGIT_POSITION-1 */
#endif

  //void (* display_scan_fn) (void);

  //void (* op_fcn [1024])(int opcode);

  // ROM:
  /*
  uint8_t bank_exists [MAX_PAGE];  // bitmap
  bool bank;                       // only a single global bank bit
  */
  const uint8_t *rom;
  const uint8_t *rom_ex;
  /*
  bool *rom_exists;
  bool *rom_breakpoint;
  */

  // RAM:
  int16_t ram_addr;  /* selected RAM address */
  creg_t ram[__USE_RAM];
} act_reg_t;

//FIXES
/*

gcc wood.c digit_ops.c

keys layout
	" SST  BST  GTO   F    G ",	// 0xb3, 0xb2, 0xb1, 0xb0, 0xb4
	" X.Y  RV   STO  RCL  E+ ",	// 0x43, 0x42, 0x41, 0x40, 0x44
	" ETR  ETR  CHS  EEX  CLX",	// 0xd3,       0xd1, 0xd0, 0xd4
	"  -     7     8      9  ",	// 0x63, 0x62, 0x61, 0x60
	"  +     4     5      6  ",	// 0xa3, 0xa2, 0xa1, 0xa0
	"  x     1     2      4  ",	// 0x73, 0x72, 0x71, 0x70
	"  /     0     .     R/S ", // 0x93, 0x92, 0x91, 0x90

*/
#define MAX_DIGIT_POSITION 15    /* Classic, Topcat, 67, maybe 19C */


#ifdef C_STANDALONE

typedef uint8_t segment_bitmap_t;		// 8 segments for our led
#define CHAR_MINUS	SEG_G
#define CHAR_DOT  	SEG_H

#else

typedef uint16_t segment_bitmap_t;		// 16bit
#define CHAR_MINUS	0x40
#define CHAR_DOT  	0x80
//static segment_bitmap_t last_display_segments[MAX_DIGIT_POSITION];
static segment_bitmap_t _display_segments[MAX_DIGIT_POSITION];
static uint16_t last_seg_crc=0;

#endif

static act_reg_t _act_reg, *act_reg=&_act_reg;

static inline uint8_t arithmetic_base (act_reg_t *act_reg) {
	return (act_reg->flags & F_DECIMAL ? 10 : 16);
}

static void bad_op (int opcode) { }

void reg_zero (digit_t *dest, int first, int last) {
	int i;
	for (i = first; i <= last; i++) dest [i] = 0;
}

void reg_copy (digit_t *dest, const digit_t *src, int first, int last) {
	int i;
	for (i = first; i <= last; i++) dest [i] = src [i];
}

void reg_exch (digit_t *dest, digit_t *src, int first, int last) {
	int i, t;
	for (i = first; i <= last; i++) {
		t = dest [i];
		dest [i] = src [i];
		src [i] = t;
	}//for
}

static digit_t do_add (digit_t x, digit_t y, uint8_t carry, uint8_t base) {
	int res;

	res = x + y + ((act_reg->flags&carry)?1:0);
	if (res >= base) {
		res -= base;
		act_reg->flags |= carry;
	}//if
	else {
		act_reg->flags &= ~carry;
	}//else
	return (res);
}


static digit_t do_sub (digit_t x, digit_t y, uint8_t carry, uint8_t base) {
	int res;

	res = (x - y) - ((act_reg->flags&carry)?1:0);
	if (res < 0) {
		res += base;
		act_reg->flags |= carry;
	}//if
	else {
		act_reg->flags &= ~carry;
	}//else
	return (res);
}

void reg_add (digit_t *dest, const digit_t *src1, const digit_t *src2, int first, int last, uint8_t carry, uint8_t base) {
	int i;

	for (i = first; i <= last; i++) {
		int s2 = src2 ? src2[i] : 0;
		dest [i] = do_add (src1[i], s2, carry, base);
	}//for
}


void reg_sub (digit_t *dest, const digit_t *src1, const digit_t *src2, int first, int last, uint8_t carry, uint8_t base) {
	int i;

	for (i = first; i <= last; i++) {
		int s1 = src1 ? src1[i] : 0;
		int s2 = src2 ? src2[i] : 0;
		int d = do_sub (s1, s2, carry, base);
		if (dest) dest [i] = d;
	}//for
}


// $$$ if in decimal mode, do illegal digits get normalized?
void reg_test_equal (const digit_t *src1, const digit_t *src2, int first, int last, uint8_t carry) {
	int i;

	act_reg->flags |= carry;
	for (i = first; i <= last; i++) {
		int s2 = src2 ? src2 [i] : 0;
		//(*carry) &= (src1 [i] == s2);
		if ((act_reg->flags & carry) && (src1 [i] != s2))
			act_reg->flags &= ~carry;
	}//for
}


// $$$ if in decimal mode, do illegal digits get normalized?
void reg_test_nonequal (const digit_t *src1, const digit_t *src2, int first, int last, uint8_t carry) {
	int i;

	//*carry = false;
	act_reg->flags &= ~carry;
	for (i = first; i <= last; i++) {
		int s2 = src2 ? src2 [i] : 0;
		//(*carry) |= (src1 [i] != s2);
		if (src1 [i] != s2)
			act_reg->flags |= carry;
	}//for
}


void reg_shift_right (digit_t *reg, int first, int last) {
	int i;
	for (i = first; i <= last; i++) reg [i] = (i == last) ? 0 : reg [i+1];
}


void reg_shift_left (digit_t *reg, int first, int last) {
	int i;
	for (i = last; i >= first; i--) reg [i] = (i == first) ? 0 : reg [i-1];
}

static void op_arith (int opcode) {
	uint8_t op, field;
	int first = 0;
	int last = 0;

	op = opcode >> 5;
	field = (opcode >> 2) & 7;

	switch (field) {
		case 0:  /* p  */
			first = act_reg->p; last = act_reg->p;
			if (act_reg->p >= WSIZE) last = 0;  /* don't do anything */
			break;
		case 1:  /* wp */
			first = 0; last = act_reg->p;
			if (act_reg->p >= WSIZE) last = WSIZE - 1;
			break;
		case 2:  /* xs */  first = EXPSIZE - 1; last = EXPSIZE - 1; break;
		case 3:  /* x  */  first = 0;           last = EXPSIZE - 1; break;
		case 4:  /* s  */  first = WSIZE - 1;   last = WSIZE - 1;   break;
		case 5:  /* m  */  first = EXPSIZE;     last = WSIZE - 2;   break;
		case 6:  /* w  */  first = 0;           last = WSIZE - 1;   break;
		case 7:  /* ms */  first = EXPSIZE;     last = WSIZE - 1;   break;
	}//switch

	//act_reg->carry = 0;
	act_reg->flags &= ~F_CARRY;

	switch (op) {
		case 0x00:  /* 0 -> a[f] */
			reg_zero (act_reg->a, first, last); break;
		case 0x01:  /* 0 -> b[f] */
			reg_zero (act_reg->b, first, last); break;
		case 0x02:  /* a exchange b[f] */
			reg_exch (act_reg->a, act_reg->b, first, last); break;
		case 0x03:  /* a -> b[f] */
			reg_copy (act_reg->b, act_reg->a, first, last); break;
		case 0x04:  /* a exchange c[f] */
			reg_exch (act_reg->a, act_reg->c, first, last); break;
		case 0x05:  /* c -> a[f] */
			reg_copy (act_reg->a, act_reg->c, first, last); break;
		case 0x06:  /* b -> c[f] */
			reg_copy (act_reg->c, act_reg->b, first, last); break;
		case 0x07:  /* b exchange c[f] */
			reg_exch (act_reg->b, act_reg->c, first, last); break;
		case 0x08:  /* 0 -> c[f] */
			reg_zero (act_reg->c, first, last); break;
		case 0x09:  /* a + b -> a[f] */
			reg_add (act_reg->a, act_reg->a, act_reg->b, first, last, F_CARRY, arithmetic_base (act_reg)); break;
		case 0x0a:  /* a + c -> a[f] */
			reg_add (act_reg->a, act_reg->a, act_reg->c, first, last, F_CARRY, arithmetic_base (act_reg)); break;
		case 0x0b:  /* c + c -> c[f] */
			reg_add (act_reg->c, act_reg->c, act_reg->c, first, last, F_CARRY, arithmetic_base (act_reg)); break;
		case 0x0c:  /* a + c -> c[f] */
			reg_add (act_reg->c, act_reg->a, act_reg->c, first, last, F_CARRY, arithmetic_base (act_reg)); break;
		case 0x0d:  /* a + 1 -> a[f] */
			//act_reg->carry = 1; 
			act_reg->flags |= F_CARRY;
			reg_add (act_reg->a, act_reg->a, NULL, first, last, F_CARRY, arithmetic_base (act_reg)); break;
		case 0x0e:  /* shift left a[f] */
			reg_shift_left (act_reg->a, first, last); break;
		case 0x0f:  /* c + 1 -> c[f] */
			//act_reg->carry = 1;
			act_reg->flags |= F_CARRY;
			reg_add (act_reg->c, act_reg->c, NULL, first, last, F_CARRY, arithmetic_base (act_reg));
			break;
		case 0x10:  /* a - b -> a[f] */
			reg_sub (act_reg->a, act_reg->a, act_reg->b, first, last, F_CARRY, arithmetic_base (act_reg)); break;
		case 0x11:  /* a - c -> c[f] */
			reg_sub (act_reg->c, act_reg->a, act_reg->c, first, last, F_CARRY, arithmetic_base (act_reg)); break;
		case 0x12:  /* a - 1 -> a[f] */
			//act_reg->carry = 1;
			act_reg->flags |= F_CARRY;
			reg_sub (act_reg->a, act_reg->a, NULL, first, last, F_CARRY, arithmetic_base (act_reg));
			break;
		case 0x13:  /* c - 1 -> c[f] */
			//act_reg->carry = 1;
			act_reg->flags |= F_CARRY;
			reg_sub (act_reg->c, act_reg->c, NULL, first, last, F_CARRY, arithmetic_base (act_reg));
			break;
		case 0x14:  /* 0 - c -> c[f] */
			reg_sub (act_reg->c, NULL, act_reg->c, first, last, F_CARRY, arithmetic_base (act_reg)); break;
		case 0x15:  /* 0 - c - 1 -> c[f] */
			//act_reg->carry = 1;
			act_reg->flags |= F_CARRY;
			reg_sub (act_reg->c, NULL, act_reg->c, first, last, F_CARRY, arithmetic_base (act_reg));
			break;
		case 0x16:  /* if b[f] = 0 */
			act_reg->inst_state = branch;
			reg_test_nonequal (act_reg->b, NULL, first, last, F_CARRY);
			break;
		case 0x17:  /* if c[f] = 0 */
			act_reg->inst_state = branch;
			reg_test_nonequal (act_reg->c, NULL, first, last, F_CARRY);
			break;
		case 0x18:  /* if a >= c[f] */
			act_reg->inst_state = branch;
			reg_sub (NULL, act_reg->a, act_reg->c, first, last, F_CARRY, arithmetic_base (act_reg));
			break;
		case 0x19:  /* if a >= b[f] */
			act_reg->inst_state = branch;
			reg_sub (NULL, act_reg->a, act_reg->b, first, last, F_CARRY, arithmetic_base (act_reg));
			break;
		case 0x1a:  /* if a[f] # 0 */
			act_reg->inst_state = branch;
			reg_test_equal (act_reg->a, NULL, first, last, F_CARRY);
			break;
		case 0x1b:  /* if c[f] # 0 */
			act_reg->inst_state = branch;
			reg_test_equal (act_reg->c, NULL, first, last, F_CARRY);
			break;
		case 0x1c:  /* a - c -> a[f] */
			reg_sub (act_reg->a, act_reg->a, act_reg->c, first, last, F_CARRY, arithmetic_base (act_reg)); break;
		case 0x1d:  /* shift right a[f] */
			reg_shift_right (act_reg->a, first, last); break;
		case 0x1e:  /* shift right b[f] */
			reg_shift_right (act_reg->b, first, last); break;
		case 0x1f:  /* shift right c[f] */
			reg_shift_right (act_reg->c, first, last); break;
	}//switch
}


static void handle_del_rom (act_reg_t *act_reg) {
	//if (act_reg->del_rom_flag) {
	if (act_reg->flags & F_DEL_ROM) {
		act_reg->pc = (act_reg->del_rom << 8) + (act_reg->pc & 0377);
		//act_reg->del_rom_flag = 0;
		act_reg->flags &= ~F_DEL_ROM;
	}//if
}


static void op_goto (int opcode) {
	//if (! act_reg->prev_carry) {
	if (!(act_reg->flags & F_PREV_CARRY)) {
		act_reg->pc = (act_reg->pc & ~0377) | (opcode >> 2);
		handle_del_rom (act_reg);
	}//if
}


static void op_jsb (int opcode) {
	act_reg->stack [act_reg->sp] = act_reg->pc;
	act_reg->sp++;
	if (act_reg->sp >= STACK_SIZE) act_reg->sp = 0;
	act_reg->pc = (act_reg->pc & ~0377) | (opcode >> 2);
	handle_del_rom (act_reg);
}

static void op_return (int opcode) {
	act_reg->sp--;
	if (act_reg->sp < 0) act_reg->sp = STACK_SIZE - 1;
	act_reg->pc = act_reg->stack [act_reg->sp];
}

static void op_nop (int opcode) { }

static void op_binary (int opcode) {
	//act_reg->decimal = false;
	act_reg->flags &= ~(F_DECIMAL);
}

static void op_decimal (int opcode) {
	//act_reg->decimal = true;
	act_reg->flags |= F_DECIMAL;
}

/* $$$ woodstock doc says when increment or decrement P wraps,
 * P "disappears for one word time". */

static void op_dec_p (int opcode) {
	if (act_reg->p)
		act_reg->p--;
	else
		act_reg->p = WSIZE - 1;
}

static void op_inc_p (int opcode) {
	act_reg->p++;
	if (act_reg->p >= WSIZE) act_reg->p = 0;
}

static void op_clear_s (int opcode) {
	int i;
	for (i = 0; i < SSIZE; i++)
		if ((i != 1) && (i != 2) && (i != 5) && (i != 15)) act_reg->s &= ~(1<<i);
}

static void op_mx(int opcode) {
	//0100, 0101, 0110, 0111
	//op_m1_exch_c, op_m1_to_c, op_m2_exch_c, op_m2_to_c,
	opcode >>= 6;
	digit_t *target = opcode&0x02 ? act_reg->m2 : act_reg->m1;
	opcode &= 0x01;
	int i;
	int t=0;
	for (i=0;i<WSIZE; i++) {
		if (i&0x01) {
			t |= act_reg->c[i]<<4;
			act_reg->c[i] = (target[i/2]>>4);
			act_reg->c[i-1] = (target[i/2]&0x0f);
			if (!opcode) target[i/2] = t;
		}//if
		else {
			t = act_reg->c[i]&0x0f;
		}//else
	}//for
}


static void op_f_to_a (int opcode) {
	act_reg->a [0] = act_reg->f;
}

static void op_f_exch_a (int opcode) {
	int t;
	t = act_reg->a [0];
	act_reg->a [0] = act_reg->f;
	act_reg->f = t;
}

static void op_circulate_a_left (int opcode) {
	int i, t;
	t = act_reg->a [WSIZE - 1];
	for (i = WSIZE - 1; i >= 1; i--)
		act_reg->a [i] = act_reg->a [i - 1];
	act_reg->a [0] = t;
}

static void op_bank_switch (int opcode) {
	//act_reg->bank ^= 1;
	act_reg->flags ^= F_BANK;
}

static void op_rom_selftest(int opcode) {
	//act_reg->crc = 01777;
	act_reg->inst_state = selftest;
	act_reg->pc &= ~ 01777;  // start from beginning of current 1K ROM bank
	//printf ("starting ROM CRC of bank %d addr %04o\n\r", act_reg->bank, act_reg->pc);
}

static void rom_selftest_done() {
	// ROM self-test completed, return and set S5 if error
	//printf ("ROM CRC done, crc = %03x: %s\n\r", act_reg->crc, act_reg->crc == 0x078 ? "good" : "bad");
	//if (act_reg->crc != 0x078) act_reg->s [5] = 1;  // indicate error
	act_reg->inst_state = norm;
	op_return (0);
}

static void op_c_to_addr (int opcode) {
	act_reg->ram_addr = (act_reg->c [1] << 4) + act_reg->c [0];
}


static void op_c_to_data (int opcode) {
	int i;
	if (act_reg->ram_addr >= __USE_RAM) return;
	//for (i = 0; i < WSIZE; i++) act_reg->ram [act_reg->ram_addr] [i] = act_reg->c [i];
	for (i=0;i<WSIZE/2;i++) act_reg->ram[act_reg->ram_addr][i] = (act_reg->c[i*2+1]<<4) | (act_reg->c[i*2]&0x0f);
}

/*
static void op_data_to_c (int opcode) {
	int i;
	if (act_reg->ram_addr >= __USE_RAM) {
		for (i = 0; i < WSIZE; i++) act_reg->c [i] = 0;
		return;
	}//if
	//for (i = 0; i < WSIZE; i++) act_reg->c [i] = act_reg->ram [act_reg->ram_addr] [i];
	for (i=0;i<WSIZE/2;i++) {
		act_reg->c[i*2+1] = act_reg->ram[act_reg->ram_addr][i]>>4;
		act_reg->c[i*2] = act_reg->ram[act_reg->ram_addr][i]&0x0f;
	}//for
}
*/

static void op_c_to_register (int opcode) {
	int i;
	act_reg->ram_addr &= ~017;
	act_reg->ram_addr += (opcode >> 6);
	if (act_reg->ram_addr >= __USE_RAM) return;
	//for (i = 0; i < WSIZE; i++) act_reg->ram [act_reg->ram_addr] [i] = act_reg->c [i];
	for (i=0;i<WSIZE/2;i++) act_reg->ram[act_reg->ram_addr][i] = (act_reg->c[i*2+1]<<4) | (act_reg->c[i*2]&0x0f);
}

static void op_register_to_c (int opcode) {
	int i;
	if (opcode>>6) {
		act_reg->ram_addr &= ~017;
		act_reg->ram_addr += (opcode >> 6);
	}//if
	else {
		// this is the same as op_data_to_c()
	}//else
	if (act_reg->ram_addr >= __USE_RAM) {
		for (i = 0; i < WSIZE; i++) act_reg->c [i] = 0;
		return;
	}//if
	//for (i = 0; i < WSIZE; i++) act_reg->c [i] = act_reg->ram [act_reg->ram_addr] [i];
	for (i=0;i<WSIZE/2;i++) {
		act_reg->c[i*2+1] = act_reg->ram[act_reg->ram_addr][i]>>4;
		act_reg->c[i*2] = act_reg->ram[act_reg->ram_addr][i]&0x0f;
	}//for
}

static void op_clear_data_regs (int opcode) {
	int base;
	int i, j;
	base = act_reg->ram_addr & ~ 017;
	for (i = base; i <= base + 15; i++)
		for (j = 0; j < WSIZE/2; j++) act_reg->ram [i] [j] = 0;
}

static void op_c_to_stack (int opcode)
{
  int i;

  for (i = 0; i < WSIZE/2; i++)
    {
      act_reg->t [i] = act_reg->z [i];
      act_reg->z [i] = act_reg->y [i];
      act_reg->y [i] = (act_reg->c[i*2]&0x0f) | (act_reg->c[i*2+1]<<4);
    }
}


static void op_stack_to_a (int opcode)
{
  int i;

  for (i = 0; i < WSIZE/2; i++)
    {
      act_reg->a [i*2] = act_reg->y[i]&0x0f;
      act_reg->a [i*2+1] = act_reg->y[i]>>4;
      act_reg->y [i] = act_reg->z [i];
      act_reg->z [i] = act_reg->t [i];
    }
}


static void op_y_to_a (int opcode)
{
  int i;

  for (i = 0; i < WSIZE/2; i++)
    {
      //act_reg->a [i] = act_reg->y [i];
      act_reg->a [i*2] = act_reg->y[i]&0x0f;
      act_reg->a [i*2+1] = act_reg->y[i]>>4;
    }
}


static void op_down_rotate (int opcode)
{
  int i, t;

  for (i = 0; i < WSIZE/2; i++)
    {
      //t = act_reg->c [i];
      //act_reg->c [i] = act_reg->y [i];
      t = (act_reg->c[i*2]&0x0f) | (act_reg->c[i*2+1]<<4);
      act_reg->c [i*2] = act_reg->y[i]&0x0f;
      act_reg->c [i*2+1] = act_reg->y[i]>>4;
      act_reg->y [i] = act_reg->z [i];
      act_reg->z [i] = act_reg->t [i];
      act_reg->t [i] = t;
    }
}


static void op_clear_reg (int opcode)
{
  int i;

  for (i = 0; i < WSIZE; i++)
    act_reg->a [i] = act_reg->b [i] = act_reg->c [i];	// = act_reg->y [i] = act_reg->z [i] = act_reg->t [i] = 0;
  for (i = 0; i < WSIZE/2; i++)
    act_reg->y [i] = act_reg->z [i] = act_reg->t [i] = 0;
  // Apparently we're not supposed to clear F, or the HP-21 CLR function
  // resets the display format.
  // Should this clear P?  Probably not.
}

static void op_load_constant (int opcode) {
	if (act_reg->p < WSIZE) act_reg->c [act_reg->p] = opcode >> 6;
	if (act_reg->p) act_reg->p--;
	else act_reg->p = WSIZE - 1;
}

static void op_set_s (int opcode) {
	if ((opcode >> 6) < SSIZE) act_reg->s |= (1<<(opcode >> 6));
}

static void op_clr_s (int opcode) {
	if ((opcode >> 6) < SSIZE) act_reg->s &= ~(1<<(opcode >> 6));
}

static void op_test_s_eq_0 (int opcode) {
	act_reg->inst_state = branch;
	//act_reg->carry = act_reg->s & (1<<(opcode >> 6)) ? true : false;
	if (act_reg->s & (1<<(opcode >> 6)))
		act_reg->flags |= F_CARRY;
	else
		act_reg->flags &= ~F_CARRY;
}

static void op_test_s_eq_1 (int opcode) {
	act_reg->inst_state = branch;
	//act_reg->carry = act_reg->s & (1<<(opcode >> 6)) ? false : true;
	if (act_reg->s & (1<<(opcode >> 6)))
		act_reg->flags &= ~F_CARRY;
	else
		act_reg->flags |= F_CARRY;
}

static const 
//__attribute__ ((section (".data_fe"))) 
uint8_t p_set_map[16] = { 14,  4,  7,  8, 11,  2, 10, 12,  1,  3, 13,  6,  0,  9,  5, 14 };
static const 
//__attribute__ ((section (".data_fe"))) 
uint8_t p_test_map[16] = {  4,  8, 12,  2,  9,  1,  6,  3,  1, 13,  5,  0, 11, 10,  7,  4 };

static void op_set_p (int opcode) {
	act_reg->p = p_set_map [opcode >> 6];
}

static void op_test_p_eq (int opcode) {
	act_reg->inst_state = branch;
	//act_reg->carry = ! (act_reg->p == p_test_map [opcode >> 6]);
	if (act_reg->p == p_test_map [opcode >> 6])
		act_reg->flags &= ~F_CARRY;
	else
		act_reg->flags |= F_CARRY;
}

static void op_test_p_ne (int opcode) {
	act_reg->inst_state = branch;
	//act_reg->carry = ! (act_reg->p != p_test_map [opcode >> 6]);
	if (act_reg->p != p_test_map [opcode >> 6])
		act_reg->flags &= ~F_CARRY;
	else
		act_reg->flags |= F_CARRY;
}

static void op_sel_rom (int opcode) {
	act_reg->pc = ((opcode & 01700) << 2) + (act_reg->pc & 0377);
}

static void op_del_sel_rom (int opcode) {
	act_reg->del_rom = opcode >> 6;
	//act_reg->del_rom_flag = 1;
	act_reg->flags |= F_DEL_ROM;
}

static void op_keys_to_rom_addr (int opcode) {
	act_reg->pc = act_reg->pc & ~0377;
	handle_del_rom (act_reg);
	if (!act_reg->key_buf) return;
	act_reg->pc += act_reg->key_buf;
}

static void op_keys_to_a (int opcode) {
	if (!act_reg->key_buf) {
		act_reg->a [2] = 0;
		act_reg->a [1] = 0;
		return;
	}//if
	act_reg->a [2] = act_reg->key_buf >> 4;
	act_reg->a [1] = act_reg->key_buf & 0x0f;
}

static void op_a_to_rom_addr (int opcode) {
	act_reg->pc = act_reg->pc & ~0377;
	handle_del_rom (act_reg);
	act_reg->pc += ((act_reg->a [2] << 4) + act_reg->a [1]);
}

static void op_display_off (int opcode) {
	//act_reg->display_enable = 0;
	act_reg->flags &= ~F_DISPLAY_ON;
}

static void op_display_toggle (int opcode) {
	//act_reg->display_enable = ! act_reg->display_enable;
	act_reg->flags ^= F_DISPLAY_ON;
}

/* doesn't seem to be in use, I need flash / ram
static void op_display_reset_twf (int opcode) {
	//act_reg->display_14_digit = true;
	act_reg->flags |= F_DISPLAY_14;
	C_RIGHT_SCAN = 0;
}
*/

static void op_crc_clear_f1 (int opcode) {
	// don't do anything, as CRC F1 is controlled by hardware
	// (in our case, ext_flag [1])
	;  
}

static void op_crc_test_f1 (int opcode) {
	if (act_reg->ext_flag & (1<<1)) act_reg->s |= (1<<3);
}

static void (* const _op_fcn [64])(int) = {
	op_nop, op_keys_to_rom_addr, op_sel_rom, bad_op,   
	bad_op, op_keys_to_a, op_sel_rom, bad_op,   
	bad_op, op_a_to_rom_addr, op_sel_rom, bad_op,        
	op_crc_test_f1, op_nop/*op_display_reset_twf*/, op_sel_rom, bad_op,            
	op_nop, op_binary, op_sel_rom, bad_op,        
	bad_op, op_circulate_a_left, op_sel_rom, op_nop,           
	bad_op, op_dec_p, op_sel_rom, bad_op,       
	bad_op, op_inc_p, op_sel_rom, bad_op,              
	op_nop, op_return, op_sel_rom, op_bank_switch, 
	bad_op, bad_op, op_sel_rom, op_c_to_addr,        
	bad_op, bad_op, op_sel_rom, op_clear_data_regs, 
	op_nop, bad_op, op_sel_rom, op_c_to_data,   
	bad_op, bad_op, op_sel_rom, op_rom_selftest,  
	op_crc_clear_f1, bad_op, op_sel_rom, bad_op,             
	bad_op, bad_op, op_sel_rom, bad_op,             
	bad_op, bad_op, op_sel_rom, op_nop,         
};

#ifdef C_STANDALONE

static const uint8_t *_key_map=0;

static void sim_check_key(uint8_t key) {
	if (_key_map[key]) {
		//woodstock_press_key(_key_map[key]);
		act_reg->key_buf = _key_map[key];
		act_reg->flags |= F_KEY;
	}//if
}

static uint8_t sim_load_segments_woodstock(uint8_t digit) {
	if (!(act_reg->flags & F_DISPLAY_ON)) return 0;
	uint8_t segs=0, i=13-digit;
	if (act_reg->b[i] & 0x02) {
		if (act_reg->a[i] == 9) segs = CHAR_MINUS;
	}//if
	else {
		segs = seg_map[act_reg->a[i]];
		if (act_reg->b[i] & 0x01) segs |= CHAR_DOT;
	}//else
	return segs;
}

static uint8_t sim_load_segments_spice(uint8_t digit) {
	if (!(act_reg->flags & F_DISPLAY_ON)) return 0;
	uint8_t segs=0, i=13-digit;
	if (digit < 11) {			// 12th digit blank?
		if (digit == 0) {
			if (act_reg->b[12] & 0x04) segs = CHAR_MINUS;
		}//if
		else {
			if (act_reg->b[i] == 6) {
				if (act_reg->a[i] == 9) segs |= CHAR_MINUS;
			}//if
			else {
				segs = act_reg->a[i];
				if (segs==0x0b) segs = 0x10;		// show 'H' instead of 'F'
				segs = seg_map[segs];
				if (act_reg->b[i] & 0x01) segs |= CHAR_DOT;	// should be dot and comma, oh well
			}//else
		}//else
	}//if
	return segs;
}

static uint8_t (*sim_load_segments) (uint8_t digit) = sim_load_segments_woodstock;

#else

static void display_scan_advance () {
	if ((--act_reg->display_scan_position) < C_RIGHT_SCAN) {
		while (act_reg->display_digit_position < MAX_DIGIT_POSITION)
			_display_segments [act_reg->display_digit_position++] = 0;
		uint16_t seg_crc=0;
		uint8_t i;
		for (i=0;i<MAX_DIGIT_POSITION;i++) seg_crc += i + _display_segments[i];
		if (seg_crc != last_seg_crc) {
			last_seg_crc = seg_crc;
		/*
		int i;
		for (i=0;i<MAX_DIGIT_POSITION;i++)
			if (last_display_segments[i] != _display_segments[i]) break;
		if (i < MAX_DIGIT_POSITION) {
		*/
#ifdef EMBEDDED
#else
            //
            printf("%02d<-->%02d\n\r", C_LEFT_SCAN, C_RIGHT_SCAN);
            putchar(' '); putchar(' '); putchar(' ');
            for (i=0;i<14;i++) { putchar("0123456789abcdef"[act_reg->a[i]&0x0f]); putchar(' '); }
            putchar('\n'); putchar('\r');
            putchar(' '); putchar(' '); putchar(' ');
            for (i=0;i<14;i++) { putchar("0123456789abcdef"[act_reg->b[i]&0x0f]); putchar(' '); }
            putchar('\n'); putchar('\r');
#endif
            //
			putchar(act_reg->flags & F_DISPLAY_ON ? '*' : ' ');
#ifdef C_SPICE
			putchar('3');
#else
			putchar('2');
#endif
			putchar('>');
			for (i=0;i<MAX_DIGIT_POSITION;i++) {
				if (_display_segments[i]&CHAR_MINUS) putchar('-');
				else putchar(" 0123456789rFope "[_display_segments[i]&0x0f]); 
				putchar((_display_segments[i]&CHAR_DOT) ? '.' : ' ');
			}//for
#ifdef EMBEDDED
#else
			putchar('\n'); 
#endif
			putchar('\r');
		}//if
		//for (i=0;i<MAX_DIGIT_POSITION;i++) last_display_segments[i] = _display_segments[i];
		//
		act_reg->display_digit_position = 0;
		act_reg->display_scan_position = C_LEFT_SCAN;
	}//if
}

static void woodstock_display_scan () {
	int a = act_reg->a [act_reg->display_scan_position];
	int b = act_reg->b [act_reg->display_scan_position];
	segment_bitmap_t segs = 0;

	if ((act_reg->flags & F_DISPLAY_14) && (act_reg->display_digit_position == 0))
		// save room for mantissa sign
		_display_segments [act_reg->display_digit_position++] = 0;

	if (act_reg->flags & F_DISPLAY_ON) {
		if (b & 2) {
			if ((a >= 2) && ((a & 7) != 7))
			segs = CHAR_MINUS;
		}//if
		else {
			segs = a+1;
		}//else
		if ((act_reg->flags & F_DISPLAY_14) && (act_reg->display_digit_position == 12)) {
			if (segs & (1 << 4)) _display_segments[0] = CHAR_MINUS;
			// exponent sign digit only has G segment
			segs &= CHAR_MINUS;
		}//if
		if (b & 1) segs |= CHAR_DOT;
	}//if

	_display_segments[act_reg->display_digit_position++] = segs;
	display_scan_advance ();
}

static void spice_display_scan() {
	int a = act_reg->a [act_reg->display_scan_position];
	int b = act_reg->b [act_reg->display_scan_position];
	segment_bitmap_t segs = 0;

	if (!act_reg->display_digit_position)
		_display_segments [act_reg->display_digit_position++] = 0;  /* make room for sign */

	if (act_reg->flags & F_DISPLAY_ON) {
		if ((act_reg->display_scan_position == /*C_LEFT_SCAN*/14-1) && (b & 4))
		_display_segments[0] = CHAR_MINUS;
		if (b == 6) {
			if (a == 9) segs = CHAR_MINUS;
		}//if
		else {
			segs = a+1;
		}//else
		if (b & 1) segs |= CHAR_DOT;		// we don't have comma segment
		//segs |= sim->char_gen [(b & 2) ? ',' : '.'];
    }//if
	_display_segments [act_reg->display_digit_position++] = segs;
	display_scan_advance ();
}

#endif

static bool woodstock_execute_cycle () {
	uint16_t opcode;
	inst_state_t prev_inst_state;

	act_reg->prev_pc = act_reg->pc;
	uint16_t idx = act_reg->pc;
	if (act_reg->flags&F_BANK) idx += 4096;
	opcode = (act_reg->rom_ex[idx/4] >> ((idx%4)*2)) & 0x03;
	opcode |= act_reg->rom[idx]<<2;

	prev_inst_state = act_reg->inst_state;
	if (act_reg->inst_state == branch) act_reg->inst_state = norm;

	if (act_reg->flags & F_CARRY)
		act_reg->flags |= F_PREV_CARRY;
	else
		act_reg->flags &= ~F_PREV_CARRY;
	act_reg->flags &= ~F_CARRY;

	if (act_reg->flags & F_KEY) act_reg->s |= (1<<15);
	if (act_reg->ext_flag & (1<<3)) act_reg->s |= (1<<3);
	if (act_reg->ext_flag & (1<<5)) act_reg->s |= (1<<5);
	act_reg->pc++;

static void (* const _op_fcn_0100 [])(int) = {	op_set_s, op_test_s_eq_1, op_test_p_eq, op_del_sel_rom, };
static void (* const _op_fcn_0300 [])(int) = {	op_clr_s, op_test_s_eq_0, op_test_p_ne, op_set_p, };
static void (* const _op_fcn_02xx [])(int) = {	op_nop, op_load_constant, op_c_to_register, op_register_to_c, };     
static void (* const _op_fcn_0200 [])(int) = {	
	op_clear_reg, op_clear_s, op_display_toggle, op_display_off, op_mx, op_mx, op_mx, op_mx,
	op_stack_to_a, op_down_rotate, op_y_to_a, op_c_to_stack, op_decimal, bad_op, op_f_to_a, op_f_exch_a,
};

	switch (prev_inst_state) {
		case norm:
			//(* act_reg->op_fcn [opcode]) (opcode);
			switch (opcode&0x0003) {
				case 0: 
					switch ((opcode>>2)&0x0003) {
						case 0: _op_fcn [(opcode>>4)] (opcode); break;
						case 1: _op_fcn_0100 [(opcode>>4)&0x03] (opcode); break;
						//case 2: _op_fcn [(opcode>>4)*2+1] (opcode); break;
						case 2: 
							if ((opcode>>4)&0x0003)
								_op_fcn_02xx [(opcode>>4)&0x03] (opcode);
							else
								_op_fcn_0200 [(opcode>>6)] (opcode); 
							break;
						case 3: _op_fcn_0300 [(opcode>>4)&0x03] (opcode); break;
					}//switch
					break;
				//case 0: _op_fcn [opcode>>2] (opcode); break;
				case 1: op_jsb(opcode); break;
				case 2: op_arith(opcode); break;
				case 3: op_goto(opcode); break;
			}//switch
			break;
		case branch:
			if (!(act_reg->flags & F_PREV_CARRY))
				act_reg->pc = (act_reg->pc & ~01777) | opcode;
			break;
		case selftest:
			//crc_update(opcode);
			if (opcode == 01060) op_bank_switch(opcode); 
			if (!(act_reg->pc & 01777)) rom_selftest_done();
			break;
	}//switch

	//act_reg->display_scan_fn ();
#ifdef C_STANDALONE
	// pcb version led scanning handled via call-back
#else

#ifdef C_SPICE
	spice_display_scan ();
#else
	woodstock_display_scan ();
#endif

#endif

	return (true);  /* never sleeps */
}

static bool woodstock_execute_instruction () {
	do {
		if (! woodstock_execute_cycle ()) return false;
	} while (act_reg->inst_state != norm);
	return true;
}

/*
static void woodstock_press_key (uint8_t keycode) {
	act_reg->key_buf = keycode;
	act_reg->flags |= F_KEY;
}
*/

static void woodstock_release_key () {
	act_reg->flags &= ~F_KEY;
}

static void woodstock_set_ext_flag (int flag, bool state) {
	if (state)
		act_reg->ext_flag |= (1<<flag);
	else
		act_reg->ext_flag &= ~(1<<flag);
	//act_reg->ext_flag [flag] = state;
}

static void woodstock_reset () {
	//act_reg->decimal = true;
	act_reg->flags = 0x00;
	act_reg->flags |= F_DECIMAL;

	act_reg->pc = 0;
	//act_reg->del_rom_flag = 0;
	act_reg->flags |= F_DEL_ROM;

	act_reg->inst_state = norm;
	act_reg->sp = 0;

	op_clear_reg(0);
	op_clear_s(0);
	act_reg->p = 0;

	act_reg->flags &= ~F_DISPLAY_ON;
#ifdef C_STANDALONE
#else
	act_reg->display_digit_position = 0;
	act_reg->display_scan_position = WSIZE - 1;
#endif

	act_reg->key_buf = 0;  // no key has been pressed
	act_reg->flags &= ~F_KEY;
	act_reg->ext_flag |= (1<<5);  // force battery ok
}

static void woodstock_clear_memory () {
	int addr;
	for (addr = 0; addr < __USE_RAM; addr++)
		reg_zero (act_reg->ram [addr], 0, WSIZE/2 - 1);
}

static uint8_t _is_spice=0;
static void woodstock_set_rom(uint8_t which) {
	switch (which) {
		case 3:		// 21
			_is_spice = 0;
			_key_map = key_map_21;
			act_reg->rom = rom_21;
			act_reg->rom_ex = rom_21_ex;
			break;
		case 2:		// 25
			_is_spice = 0;
			_key_map = key_map_25c;
			act_reg->rom = rom_25c;
			act_reg->rom_ex = rom_25c_ex;
			break;
		default:
		case 1:		// 33
			_is_spice = 1;
			_key_map = key_map_33c;
			act_reg->rom = rom_33c;
			act_reg->rom_ex = rom_33c_ex;
			break;
	}//switch
}

static void woodstock_new_processor() {
	//act_reg = malloc (sizeof (act_reg_t));
#ifdef C_SPICE
	//C_display_scan_fn = spice_display_scan;
	//C_LEFT_SCAN = WSIZE - 2;
	//C_RIGHT_SCAN = 3;
#else
	//act_reg->display_scan_fn = woodstock_display_scan;
	//C_LEFT_SCAN = WSIZE - 1;
	//C_RIGHT_SCAN = 2;
#endif

#ifdef C_STANDALONE
	sim_load_segments = _is_spice ? sim_load_segments_spice : sim_load_segments_woodstock;
#else
	act_reg->display_scan_position = C_LEFT_SCAN;
	act_reg->display_digit_position = 0;
#endif
	woodstock_reset();
/* retired, now runtime slect
#ifdef C_SPICE
	act_reg->rom = rom_33c;
	act_reg->rom_ex = rom_33c_ex;
#else
	act_reg->rom = rom_25c;
	act_reg->rom_ex = rom_25c_ex;
#endif
*/
	woodstock_clear_memory();
}

#ifdef EMBEDDED

#ifdef C_STANDALONE
void set_conio_terminal_mode() { }
#endif
int kbhit() { return 0; }
int getch() { return 0; }

#else
//_______________________________ stuff for key catching
struct termios orig_termios;

void reset_terminal_mode() {
    tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode() {
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}

int kbhit() {
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

int getch() {
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0) {
        return r;
    } else {
        return c;
    }
}

#endif
