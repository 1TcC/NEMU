#include "cpu/exec/template-start.h"

#define instr je

static void do_execute() {
	DATA_TYPE_S offset = (DATA_TYPE_S)op_src->val;

	if(cpu.eflags.ZF) {
		cpu.eip += offset;
	}

	print_asm("je %x", cpu.eip + 1 + DATA_BYTE);
}

make_instr_helper(si)

#undef instr


#define instr jbe

static void do_execute() {
	DATA_TYPE_S offset = (DATA_TYPE_S)op_src->val;

	if(cpu.eflags.CF || cpu.eflags.ZF) {
		cpu.eip += offset;
	}

	print_asm("jbe %x", cpu.eip + 1 + DATA_BYTE);
}

make_instr_helper(si)

#undef instr

#include "cpu/exec/template-end.h"