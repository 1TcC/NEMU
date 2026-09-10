#include "cpu/exec/template-start.h"

#define instr call

static void do_execute() {
    cpu.esp -= 4;
    swaddr_write(cpu.esp, 4, cpu.eip + 1 + DATA_BYTE);

    cpu.eip += op_src->val;

    print_asm(str(instr) " %x",
              cpu.eip + 1 + DATA_BYTE);
}

make_instr_helper(si)

#include "cpu/exec/template-end.h"