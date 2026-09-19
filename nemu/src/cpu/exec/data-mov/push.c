#include "cpu/exec/helper.h"

#define DATA_BYTE 4
#include "push-template.h"
#undef DATA_BYTE

make_helper(push_i_b) {
    int32_t value = (int8_t)instr_fetch(eip + 1, 1);

    cpu.esp -= 4;
    swaddr_write(cpu.esp, 4, (uint32_t)value, R_SS);

    print_asm("push $0x%x", value);
    return 2;
}

make_helper(push_i_l) {
    uint32_t value = instr_fetch(eip + 1, 4);

    cpu.esp -= 4;
    swaddr_write(cpu.esp, 4, value, R_SS);

    print_asm("push $0x%x", value);
    return 5;
}
