#include "cpu/exec/helper.h"
#include "cpu/decode/modrm.h"
#include "memory/memory.h"

#define DATA_BYTE 1
#include "mov-template.h"
#undef DATA_BYTE

#define DATA_BYTE 2
#include "mov-template.h"
#undef DATA_BYTE

#define DATA_BYTE 4
#include "mov-template.h"
#undef DATA_BYTE

/* for instruction encoding overloading */

make_helper_v(mov_i2r)
make_helper_v(mov_i2rm)
make_helper_v(mov_r2rm)
make_helper_v(mov_rm2r)
make_helper_v(mov_a2moffs)
make_helper_v(mov_moffs2a)

make_helper(mov_cr2r_l) {
	ModR_M m;
	m.val = instr_fetch(eip + 1, 1);

	assert(m.mod == 3);
	assert(m.reg == 0);

	reg_l(m.R_M) = cpu.cr0.val;

	print_asm("movl %%cr0,%%%s", regsl[m.R_M]);

	return 2;
}

make_helper(mov_r2cr_l) {
	ModR_M m;
	m.val = instr_fetch(eip + 1, 1);

	assert(m.mod == 3);
	assert(m.reg == 0);

	cpu.cr0.val = reg_l(m.R_M);

	print_asm("movl %%%s,%%cr0", regsl[m.R_M]);

	return 2;
}

make_helper(mov_rm2sreg_w) {
	ModR_M m;
	m.val = instr_fetch(eip + 1, 1);

	assert(m.reg <= R_DS);
	assert(m.reg != R_CS);

	uint16_t selector;
	int len;

	if(m.mod == 3) {
		selector = reg_w(m.R_M);
		len = 1;

		static const char *sreg_name[] = {
			"es", "cs", "ss", "ds"
		};

		print_asm("movw %%%s,%%%s",
				regsw[m.R_M],
				sreg_name[m.reg]);
	}
	else {
		Operand rm;
		rm.size = 2;

		len = load_addr(eip + 1, &m, &rm);
		selector = swaddr_read(rm.addr, 2);

		static const char *sreg_name[] = {
			"es", "cs", "ss", "ds"
		};

		print_asm("movw %s,%%%s",
				rm.str,
				sreg_name[m.reg]);
	}

	cpu.sreg[m.reg].val = selector;

	load_sreg(m.reg);

	return len + 1;
}