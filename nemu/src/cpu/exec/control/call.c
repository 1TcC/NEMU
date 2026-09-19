#include "cpu/exec/helper.h"

#define DATA_BYTE 4
#include "call-template.h"
#undef DATA_BYTE

make_helper(call_rm_l) {
    int len = decode_rm_l(eip + 1) + 1;
    uint32_t target = op_src->val;

    cpu.esp -= 4;
    swaddr_write(cpu.esp, 4, eip + len, R_SS);

    cpu.eip = target - len;

    print_asm("call *%s", op_src->str);

    return len;
}