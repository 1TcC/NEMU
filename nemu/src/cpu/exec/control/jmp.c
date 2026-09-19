#include "cpu/exec/helper.h"
#include "memory/memory.h"

#define DATA_BYTE 1
#include "jmp-template.h"
#undef DATA_BYTE

#define DATA_BYTE 4
#include "jmp-template.h"
#undef DATA_BYTE

make_helper(ljmp) {
	uint32_t offset = instr_fetch(eip + 1, 4);
	uint16_t selector = instr_fetch(eip + 5, 2);

	cpu.cs.val = selector;

	load_sreg(R_CS);

	print_asm("ljmp $0x%x,$0x%x",
			selector, offset);

	cpu.eip = offset - 7;

	return 7;
}