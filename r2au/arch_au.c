/* radare2-au - MIT - Copyright 2018-2025 - pancake */

#include <r_asm.h>
#include <r_lib.h>
#include "cpu.h"

static int typeChar(const ut8 t) {
	const char *str = "sczZpntsid";
	if (t < strlen (str)) {
		return str[t];
	}
	return '?';
}

static void aucpu_esil_wave(RAnalOp *op, const ut8 *data) {
	int type = typeChar(data[1]);
	int freq = ((data[2]<< 8) | data[3]) << 2;
	if (freq == 0) {
		r_strbuf_setf (&op->esil, ","); // do nothing
	} else {
		r_strbuf_setf (&op->esil, "auw%c %d@ r0!r1,#!", type, freq);
	}
}

static bool assemble(RArchSession *a, RAnalOp *op, RArchEncodeMask mask) {
	const char *str = op->mnemonic;
	char *arg = strdup (str);
	r_str_replace_char (arg, ',', ' ');
	RList *args = r_str_split_list (arg, " ", -1);
	const char *mnemonic = r_list_get_n (args, 0);
	free (op->bytes);
	op->bytes = malloc (32);
	ut8 *buf = op->bytes;
	op->size = -1;
	if (!strcmp (mnemonic, "nop")) {
		buf[0] = AUCPU_OP_NOP;
		op->size = 4;
	} else if (!strcmp (mnemonic, "add")) {
		const char *arg0 = r_list_get_n (args, 1);
		buf[0] = AUCPU_OP_ADD;
		op->size = 4;
		if (arg0 && *arg0 == 'r') {
			buf[1] = atoi (arg0 + 1);
			const char *arg1 = r_list_get_n (args, 2);
			if (arg1) {
				if (*arg1 == 'r') {
					buf[0] = AUCPU_OP_ADDREG;
					buf[1] |= atoi (arg0 + 1);
					op->size = 2;
				} else {
					ut16 v = r_num_math (NULL, arg1);
					buf[2] = (v >> 8) & 0xff;
					buf[3] = (v & 0xff);
				}
			}
		} else {
			return false;
		}
	} else if (!strcmp (mnemonic, "mov")) {
		const char *arg0 = r_list_get_n (args, 1);
		buf[0] = AUCPU_OP_MOV;
		op->size = 4;
		if (arg0 && *arg0 == 'r') {
			buf[1] = atoi (arg0 + 1);
			const char *arg1 = r_list_get_n (args, 2);
			if (arg1) {
				if (*arg1 == 'r') {
					buf[0] = AUCPU_OP_MOVREG;
					buf[1] |= atoi (arg0 + 1);
					op->size = 2;
				} else {
					ut16 v = r_num_math (NULL, arg1);
					buf[2] = (v >> 8) & 0xff;
					buf[3] = (v & 0xff);
				}
			} else {
				return false;
			}
		} else {
			return false;
		}
	} else if (!strcmp (mnemonic, "trap")) {
		op->size = 2;
		buf[0] = AUCPU_OP_TRAP;
	} else if (!strcmp (mnemonic, "wave")) {
		op->size = 2;
		buf[0] = AUCPU_OP_WAVE;
		buf[1] = 0; // TODO: type;
		// wave r0
		return false;
		// RETHINK OP, r0, r1 must be 2nd arg
		// wsin r0, r1, r2
	} else if (!strcmp (mnemonic, "play")) {
		buf[0] = AUCPU_OP_PLAY;
		op->size = 2;
		const char *arg0 = r_list_get_n (args, 1);
		if (arg0 && *arg0 == 'r') {
			const char *arg1 = r_list_get_n (args, 2);
			if (arg1 && *arg1 == 'r') {
				ut16 v = r_num_math (NULL, arg1);
				buf[0] = AUCPU_OP_PLAYREG;
				buf[1] = atoi (arg0 + 1);
				buf[1] |= atoi (arg1 + 1) << 4;
				op->size = 2;
			}
		}
	}
	r_list_free (args);
	return op->size > 0;
}

static const char *waveType(const ut8 t) {
	const char *types[] = {
		"sin", "cos", "saw", "rsaw", "pulse",
		"noise", "triangle", "silence", "inc", "dec",
		NULL
	};
	int i = 0;
	for (i = 0; types[i] && i < t; i++) {
		// find index in bounds
	}
	return types[i];
}

static void invalid(RAnalOp *op, const ut8 *buf) {
	st16 *dword = (st16*)buf;
	op->type = R_ANAL_OP_TYPE_ILL;
	free (op->mnemonic);
	op->mnemonic = r_str_newf (".short %d", *dword);
	op->size = 2;
}

static bool au_op(RArchSession *as, RAnalOp *op, RArchDecodeMask mask) {
	if (op->size < 2) {
		return false;
	}
	const ut8 *data = op->bytes;
	op->size = 4;
	op->cycles = 1;
	switch (data[0]) {
	case AUCPU_OP_NOP:
		op->type = R_ANAL_OP_TYPE_NOP;
		break;
	case AUCPU_OP_PLAY:
		op->type = R_ANAL_OP_TYPE_MOV;
		r_strbuf_setf (&op->esil, "au.@ r0!r1,#!");
		op->cycles = 2;
		op->size = 2;
		break;
	case AUCPU_OP_PLAYREG:
		op->size = 2;
		op->type = R_ANAL_OP_TYPE_SWI;
		{
			int r0 = data[1] & 0xf;
			int r1 = (data[1] & 0xf0) >> 4;
			r_strbuf_setf (&op->esil, "au.@ r%d!r%d,#!", r0, r1);
		}
		op->cycles = 2;
		break;
	case AUCPU_OP_TRAP:
		op->type = R_ANAL_OP_TYPE_TRAP;
		op->cycles = 2;
		break;
	case AUCPU_OP_MOVREG:
		op->type = R_ANAL_OP_TYPE_MOV;
		{
			int r0 = data[1] & 0xf;
			int r1 = (data[1] & 0xf0) >> 4;
			r_strbuf_setf (&op->esil, "r%d,r%d,=", r0, r1);
		}
		break;
	case AUCPU_OP_MOV:
		op->type = R_ANAL_OP_TYPE_MOV;
		{
			int v = (data[2] << 8) | data[3];
			int r = data[1];
			r_strbuf_setf (&op->esil, "%d,r%d,=", v, r);
		}
		break;
	case AUCPU_OP_WAVE:
		op->type = R_ANAL_OP_TYPE_STORE;
		op->size = 4;
		aucpu_esil_wave (op, data);
		break;
	case AUCPU_OP_WAIT:
		op->type = R_ANAL_OP_TYPE_NOP;
		{
			int v = (data[2] << 8) | data[3];
			int r = data[1];
			op->cycles = v;
			r_strbuf_setf (&op->esil, "sleep %d,#!", v);
		}
		break;
	case AUCPU_OP_JMP:
		{
			int r0 = data[1] & 0xf;
			int r1 = (data[1] & 0xf0);
			if (r1) {
				int v = (data[2] << 8) | data[3];
				if (r1 == 0xf0) {
					r_strbuf_setf (&op->esil, "%d,pc,-=", r0);
				} else {
					r_strbuf_setf (&op->esil, "%d,pc,+,4,pc,=", r0);
				}
				op->size = 4;
			} else {
				r_strbuf_setf (&op->esil, "r%d,pc,+=", r0);
				op->size = 2;
			}
		}
		break;
	defaultr:
		r_strbuf_setf (&op->esil, "?E hello world,#!");
		break;
	}
	return true;
}

static bool disassemble(RArchSession *as, RAnalOp *op, RArchDecodeMask mask) {
	const ut8 *buf = op->bytes;
	const int len = op->size;
	au_op (as, op, mask);
	if (len < 2) {
		return false;
	}
	op->size = 4;
	switch (buf[0]) {
	case AUCPU_OP_NOP:
		op->size = 2;
		free (op->mnemonic);
		op->mnemonic = strdup ("nop");
		break;
	case AUCPU_OP_MOVREG:
		{
			int r0 = buf[1] & 0xf;
			int r1 = (buf[1] & 0xf0);
			op->size = 4;
			free (op->mnemonic);
			op->mnemonic = r_str_newf ("mov r%d, r%d", r0, r1);
		}
		break;
	case AUCPU_OP_MOV:
		{
			int r = buf[1];
			int v = (buf[2] << 8) | buf[3];
			free (op->mnemonic);
			op->mnemonic = r_str_newf ("mov r%d, %d", r, v);
			op->size = 4;
		}
		break;
	case AUCPU_OP_WAVE:
		{
			int t = buf[1];
			int freq = ((buf[2]<< 8) | buf[3]) << 2;
			op->size = 4;
			const char *type = waveType (buf[1]);
			if (type) {
				free (op->mnemonic);
				op->mnemonic = r_str_newf ("wave %s, %d", type, freq);
			} else {
				invalid (op, buf);
				return false;
			}
		}
		break;
	case AUCPU_OP_WAIT:
		{
			int r0 = buf[1] & 0xf;
			int r1 = (buf[1] & 0xf0);
			if (r1) {
				int v = (buf[2] << 8) | buf[3];
				free (op->mnemonic);
				op->mnemonic = r_str_newf ("wait %d", v);
				op->size = 4;
			} else {
				free (op->mnemonic);
				op->mnemonic = r_str_newf ("wait r%d", r0);
				op->size = 2;
			}
		}
		break;
	case AUCPU_OP_JMP:
		{
			int r0 = buf[1] & 0xf;
			int r1 = (buf[1] & 0xf0);
			if (r1) {
				int v = (buf[2] << 8) | buf[3];
				if (r1 == 0xf0) {
					v = -v;
				}
				ut64 addr = op->addr + v + 4;
				free (op->mnemonic);
				op->mnemonic = r_str_newf ("jmp 0x%08"PFMT64x, addr);
				op->size = 4;
			} else {
				free (op->mnemonic);
				op->mnemonic = r_str_newf ("jmp r%d", r0);
				op->size = 2;
			}
		}
		break;
	case AUCPU_OP_TRAP:
		free (op->mnemonic);
		op->mnemonic = r_str_newf ("trap");
		op->size = 2;
		break;
	case AUCPU_OP_PLAY: // DEPRECATE?
		free (op->mnemonic);
		op->mnemonic = strdup ("play");
		op->type = R_ANAL_OP_TYPE_MOV;
		op->size = 2;
		break;
	case AUCPU_OP_PLAYREG:
		{
			int r0 = buf[1] & 0xf;
			int r1 = (buf[1] & 0xf0) >> 4;
			free (op->mnemonic);
			op->mnemonic = r_str_newf ("play r%d, r%d", r0, r1);
		}
		break;
	default:
		eprintf ("INVALID %d\n", *buf);
		invalid (op, buf);
		return false;
	}
	return true;
}

static char *regs(RArchSession *as) {
#if 0
	// Registers
	ProgramCounter
	WavePointer - where to play
	WaveFreq - what to write
	WaveType - sin, cos, ..
	AudioBlockSize
	Frequency (22050, 44100, ..)
	BitsPerSample (8, 16, 32)
	Channels (1, 2) mono or stereo

	// General Purpose Registers
	r0-r8

	// Operations
	mov r0, 120
	mov r0, r1
	shl r0, 3
	and r0, 3
	shr r0, 3
	cj r0, r0
	mov r0, r1
	mov r0, pc
	add r0, -8
	load r4, r0 // r4=[r0]
	j r0
	j r1

#endif
	const char p[] =
		"=PC	pc\n"
		"=BP	sp\n"
		"=SP	sp\n"
		"=A0	r0\n"
		"=R0	r0\n"
		"gpr	r0	.32	0	0\n"
		"gpr	r1	.32	4	0\n"
		"gpr	r2	.32	8	0\n"
		"gpr	r3	.32	12	0\n"
		"gpr	r4	.32	16	0\n"
		"gpr	r5	.32	20	0\n"
		"gpr	r6	.32	24	0\n"
		"gpr	r7	.32	28	0\n"
		"gpr	r8	.32	32	0\n"
		"gpr	r9	.32	36	0\n"
		"gpr	r10	.32	40	0\n"
		"gpr	r11	.32	44	0\n"
		"gpr	r12	.32	48	0\n"
		"gpr	sp	.32	52	0\n"
		"gpr	bp	.32	56	0\n"
		"gpr	pc	.32	60	0\n"

		"gpr	flags	.8	.192	0\n"
		"gpr	C	.1	.192	0\n"
		"gpr	Z	.1	.193	0\n"
		"gpr	I	.1	.194	0\n"
		"gpr	D	.1	.195	0\n";
	return strdup (p);
}

static int archinfo(RArchSession *as, ut32 what) {
	switch (what) {
	case R_ARCH_INFO_INVOP_SIZE:
		return 2;
	case R_ARCH_INFO_MINOP_SIZE:
		return 2;
	case R_ARCH_INFO_MAXOP_SIZE:
		return 4;
	}
	return 2;
}

static char *mnemonics(RArchSession *as, int id, bool json) {
	char *s = strdup ("nop\n"
			"mov\n"
			"trap\n"
			"wave\n"
			"play\n"
			);
	return s;
}

RArchPlugin r_arch_plugin_au = {
	.meta = {
		.name = "au",
		.desc = "virtual audio chip",
		.license = "MIT",
	},
	.arch = "au",
	.bits = R_SYS_BITS_PACK3 (8, 16, 32),
	.endian = R_SYS_ENDIAN_LITTLE | R_SYS_ENDIAN_BIG,
	.regs = regs,
	.info = archinfo,
	.mnemonics = &mnemonics,
	.encode = &assemble,
	.decode = &disassemble,
};

#ifndef CORELIB
RLibStruct radare_plugin = {
	.type = R_LIB_TYPE_ARCH,
	.data = &r_arch_plugin_au,
	.version = R2_VERSION
};
#endif

