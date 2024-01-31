#include "x86emu.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

double fpu_stack[8];
int    fpu_st0_ptr = -1;
int    ZF          = 0;
int    CF          = 0;
int    SF          = 0;

x86_reg *eax, *ebx, *ecx, *edx, *esi, *ebp;

#define ST_(i) fpu_stack[fpu_st0_ptr - i]

// float MEM32x(float)
// {
//     return 1;
// }

void x86emu_init()
{
    eax = malloc(sizeof(x86_reg));
    ebx = malloc(sizeof(x86_reg));
    ecx = malloc(sizeof(x86_reg));
    edx = malloc(sizeof(x86_reg));
    esi = malloc(sizeof(x86_reg));
    ebp = malloc(sizeof(x86_reg));
}
void fail()
{
    exit(1);
}

x87_operand x87_op_f(float f)
{
    x87_operand o;
    o.type      = X87_OP_FLOAT;
    o.float_val = f;
    return o;
}

x87_operand x87_op_i(int i)
{
    x87_operand o;
    o.type     = X87_OP_ST;
    o.st_index = i;
    return o;
}

x86_operand x86_op_reg(x86_reg *r)
{
    x86_operand o;
    o.type = X86_OP_REG;
    o.reg  = r;
    return o;
}

x86_operand x86_op_imm(uint32_t imm)
{
    x86_operand o;
    o.type = X86_OP_IMM;
    o.imm  = imm;
    return o;
}

x86_operand x86_op_mem32(void *bytes)
{
    x86_operand o;
    o.type        = X86_OP_MEM32;
    o.mem.ptr_val = bytes;
    return o;
}

x86_operand x86_op_ptr(void *ptr)
{
    x86_operand o;
    o.type = X86_OP_PTR;
    o.ptr  = ptr;
    return o;
}

x87_operand make_x87_op(unsigned char *bytes, char type)
{
    x87_operand o;
    o.type = type;
    switch(type) {
        case X87_OP_FLOAT:
            memcpy(o.bytes, bytes, 4);
            break;
        case X87_OP_DOUBLE:
            memcpy(o.bytes, bytes, 8);
            break;
        case X87_OP_ST:
            memcpy(o.bytes, bytes, 8);
            break;
        default:
            fail();
    }
    return o;
}

void fld(x87_operand op)
{
    /*
fld(10)
0: 10
fld(20)
0: 10  <- ST(1)
1: 20  <- ST(0)
*/

    switch(op.type) {
        case X87_OP_ST:
            fpu_stack[fpu_st0_ptr + 1] = ST_(op.st_index);
            break;
        case X87_OP_FLOAT:
            fpu_stack[fpu_st0_ptr + 1] = op.float_val;
            printf("fld %f\n", op.float_val);
            break;
        case X87_OP_DOUBLE:
            fpu_stack[fpu_st0_ptr + 1] = op.double_val;
            break;
        default:
            fail();
    }
    fpu_st0_ptr++;
}

void fsub(float v)
{
    printf("fsub %f\n", v);
    ST_(0) -= v;
}

void fsub_2(x87_operand dest, x87_operand src)
{
    ST_(dest.st_index) -= ST_(src.st_index);
}

void fsubp_2(x87_operand dest, x87_operand src)
{
    fsub_2(dest, src);
    fpu_st0_ptr--;
}

void fmul_2(x87_operand dest, x87_operand src)
{
    ST_(dest.st_index) *= ST_(src.st_index);
}

void fdivr(float f)
{
    ST_(0) = f / ST_(0);
}

void mov(x86_operand dest, x86_operand src)
{
    uint8_t *src_val;
    int      size;
    switch(src.type) {
        case X86_OP_MEM32:
            src_val = src.mem.ptr_val;
            size    = 4;
            break;
        case X86_OP_REG:
            src_val = src.reg->bytes;
            size    = 4;
            break;
        case X86_OP_PTR:
            src_val = src.ptr;
            size    = 8; // TODO
            break;
        case X86_OP_IMM:
            src_val = &src.imm;
            break;
        default:
            fail();
    }

    switch(dest.type) {
        case X86_OP_REG:
            memcpy(dest.reg->bytes, src_val, size);
            break;
        case X86_OP_MEM32:
            memcpy(dest.mem.ptr_val, src_val, size);
            break;
    }
}

void xor_(x86_operand dest, x86_operand src)
{
    uint8_t *src_val;
    int      size;
    switch(src.type) {
        case X86_OP_MEM32:
            src_val = src.mem.ptr_val;
            size    = 4;
            break;
        case X86_OP_REG:
            src_val = src.reg->bytes;
            size    = 4;
            break;
        default:
            fail();
    }

    switch(dest.type) {
        case X86_OP_REG:
            for(int i = 0; i < size; i++) {
                dest.reg->bytes[i] ^= src_val[i];
            }
            break;
        case X86_OP_MEM32:
            fail();
    }
}

void cmp(x86_operand dest, x86_operand src)
{
    uint8_t *src_val;
    int      size;
    switch(src.type) {
        case X86_OP_MEM32:
            src_val = src.mem.ptr_val;
            size    = 4;
            break;
        case X86_OP_REG:
            src_val = src.reg->bytes;
            size    = 4;
            break;
        default:
            fail();
    }

    switch(dest.type) {
        case X86_OP_REG:
            if(dest.reg->uint_val < *(uint32_t *)src_val) {
                CF = 1;
                ZF = 0;
                SF = 1;
            } else if(dest.reg->uint_val == *(uint32_t *)src_val) {
                CF = 0;
                ZF = 1;
            } else {
                CF = 0;
                ZF = 0;
            }
            break;
        default:
            fail();
    }
}

void rcl(x86_operand dest, int count)
{
    switch(dest.type) {
        case X86_OP_REG:
            while(count != 0) {
                int msb = dest.reg->uint_val & 0x80000000;
                // rotate CF flag into lsb
                dest.reg->uint_val = (dest.reg->uint_val << 1) + CF;
                // rotate msb into CF
                CF = msb;
                count--;
            }
            break;
        default:
            fail();
    }
}

void sub(x86_operand dest, x86_operand src)
{
    void *src_val;
    int   size;
    switch(src.type) {
        case X86_OP_MEM32:
            src_val = src.mem.ptr_val;
            size    = 4;
            break;
        case X86_OP_REG:
            src_val = src.reg->bytes;
            size    = 4;
            break;
        default:
            fail();
    }
    switch(dest.type) {
        case X86_OP_REG:
            dest.reg->uint_val -= *(uint32_t *)src_val;
            break;
        default:
            fail();
    }
}

void and (x86_operand dest, x86_operand src)
{
    void *src_val;
    int   size;
    switch(src.type) {
        case X86_OP_MEM32:
            src_val = src.mem.ptr_val;
            size    = 4;
            break;
        case X86_OP_REG:
            src_val = src.reg->bytes;
            size    = 4;
            break;
        case X86_OP_IMM:
            src_val = &src.imm;
            break;
        default:
            fail();
    }
    switch(dest.type) {
        case X86_OP_REG:
            dest.reg->uint_val &= *(uint32_t *)src_val;
            break;
        default:
            fail();
    }
}

void or (x86_operand dest, x86_operand src)
{
    void *src_val;
    int   size;
    switch(src.type) {
        case X86_OP_MEM32:
            src_val = src.mem.ptr_val;
            size    = 4;
            break;
        case X86_OP_REG:
            src_val = src.reg->bytes;
            size    = 4;
            break;
        case X86_OP_IMM:
            src_val = &src.imm;
            break;
        default:
            fail();
    }
    switch(dest.type) {
        case X86_OP_REG:
            dest.reg->uint_val |= *(uint32_t *)src_val;
            break;
        default:
            fail();
    }
}

void shr(x86_operand dest, int count)
{
    assert(dest.type == X86_OP_REG);

    while(count != 0) {
        CF = dest.reg->uint_val & 1;
        dest.reg->uint_val /= 2; // Unsigned divide
        count--;
    }
}
