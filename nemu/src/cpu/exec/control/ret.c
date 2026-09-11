#include "cpu/exec/helper.h"

make_helper(ret) {
	uint32_t return_addr = swaddr_read(cpu.esp, 4);

	cpu.esp += 4;
	cpu.eip = return_addr - 1;

	print_asm("ret");

	return 1;
}