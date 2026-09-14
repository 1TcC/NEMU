#include "cpu/exec/template-start.h"

#define instr je
static void do_execute() {
    DATA_TYPE_S offset = (DATA_TYPE_S)op_src->val;
    if(cpu.eflags.ZF) {
        cpu.eip += offset;
    }
    print_asm("je");
}
make_instr_helper(si)
#undef instr

#define instr jne
static void do_execute() {
    DATA_TYPE_S offset = (DATA_TYPE_S)op_src->val;
    if(!cpu.eflags.ZF) {
        cpu.eip += offset;
    }
    print_asm("jne");
}
make_instr_helper(si)
#undef instr

#define instr jbe
static void do_execute() {
    DATA_TYPE_S offset = (DATA_TYPE_S)op_src->val;
    if(cpu.eflags.CF || cpu.eflags.ZF) {
        cpu.eip += offset;
    }
    print_asm("jbe");
}
make_instr_helper(si)
#undef instr

#define instr jge
static void do_execute() {
    DATA_TYPE_S offset = (DATA_TYPE_S)op_src->val;
    if(cpu.eflags.SF == cpu.eflags.OF) {
        cpu.eip += offset;
    }
    print_asm("jge");
}
make_instr_helper(si)
#undef instr

#define instr jle
static void do_execute() {
    DATA_TYPE_S offset = (DATA_TYPE_S)op_src->val;
    if(cpu.eflags.ZF || cpu.eflags.SF != cpu.eflags.OF) {
        cpu.eip += offset;
    }
    print_asm("jle");
}
make_instr_helper(si)
#undef instr

#define instr jl
static void do_execute() {
    DATA_TYPE_S offset = (DATA_TYPE_S)op_src->val;
    if(cpu.eflags.SF != cpu.eflags.OF) {
        cpu.eip += offset;
    }
    print_asm("jl");
}
make_instr_helper(si)
#undef instr

#define instr jg
static void do_execute() {
    DATA_TYPE_S offset = (DATA_TYPE_S)op_src->val;
    if(!cpu.eflags.ZF && cpu.eflags.SF == cpu.eflags.OF) {
        cpu.eip += offset;
    }
    print_asm("jg");
}
make_instr_helper(si)
#undef instr

#define instr ja
static void do_execute() {
    DATA_TYPE_S offset = (DATA_TYPE_S)op_src->val;
    if(!cpu.eflags.CF && !cpu.eflags.ZF) {
        cpu.eip += offset;
    }
    print_asm("ja");
}
make_instr_helper(si)
#undef instr

#define instr js
static void do_execute() {
    DATA_TYPE_S offset = (DATA_TYPE_S)op_src->val;

    if(cpu.eflags.SF) {
        cpu.eip += offset;
    }

    print_asm("js");
}
make_instr_helper(si)
#undef instr

#define instr jns
static void do_execute() {
    DATA_TYPE_S offset = (DATA_TYPE_S)op_src->val;

    if(!cpu.eflags.SF) {
        cpu.eip += offset;
    }

    print_asm("jns");
}
make_instr_helper(si)
#undef instr

#include "cpu/exec/template-end.h"
