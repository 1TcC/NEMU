#include "cpu/exec/helper.h"

make_helper(ret) {
	uint32_t return_addr = swaddr_read(cpu.esp, 4);

	cpu.esp += 4;
	cpu.eip = return_addr - 1;

	print_asm("ret");

	return 1;
}

make_helper(ret_i_w) {
    uint16_t imm = instr_fetch(eip + 1, 2);

    uint32_t return_addr = swaddr_read(cpu.esp, 4);
    cpu.esp += 4;
    cpu.esp += imm;

    cpu.eip = return_addr - 3;

    print_asm("ret $0x%x", imm);

    return 3;
}