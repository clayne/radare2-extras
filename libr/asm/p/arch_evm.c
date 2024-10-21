/* radare - LGPL - Copyright 2017-2023 - pancake */

#include <stdio.h>
#include <string.h>
#include <r_types.h>
#include <r_lib.h>
#include <r_asm.h>
#include "evm.h"
// #include "../arch/evm/evm.c"

static int disassemble(RAsm *a, RAsmOp *op, const ut8 *buf, int len) {
	EvmOp eop = {
		0
	};

	evm_dis (&eop, buf, len);
	op->size = eop.len;
	r_asm_op_set_asm (op, eop.txt);

	return eop.len;
}

static int assemble(RAsm *a, RAsmOp *op, const char *buf) {
	op->size = evm_asm (buf, &op->buf, sizeof (op->buf));
	return op->size;
}

RAsmPlugin r_a_plugin_evm = {
	.meta = {
		.name = "evm",
		.author = "pancake",
		.version = "0.0.1",
		.license = "MIT",
	},
	.arch = "evm",
	.bits = 32,
	.endian = R_SYS_ENDIAN_BIG,
	.desc = "evm",
	.disassemble = &disassemble,
	.assemble = &assemble
};

#ifndef CORELIB
RLibStruct radare_plugin = {
	.type = R_LIB_TYPE_ARCH,
	.data = &r_arch_plugin_evm,
	.version = R2_VERSION
};
#endif
