/*
 *    Stack-less Just-In-Time compiler
 *
 *    Copyright 2009-2012 Zoltan Herczeg (hzmester@freemail.hu). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this list of
 *      conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice, this list
 *      of conditions and the following disclaimer in the documentation and/or other materials
 *      provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDER(S) OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

SLJIT_API_FUNC_ATTRIBUTE SLJIT_CONST char* sljit_get_platform_name(void)
{
	return "x86" SLJIT_CPUINFO;
}

/*
   32b register indexes:
     0 - EAX
     1 - ECX
     2 - EDX
     3 - EBX
     4 - none
     5 - EBP
     6 - ESI
     7 - EDI
*/

/*
   64b register indexes:
     0 - RAX
     1 - RCX
     2 - RDX
     3 - RBX
     4 - none
     5 - RBP
     6 - RSI
     7 - RDI
     8 - R8   - From now on REX prefix is required
     9 - R9
    10 - R10
    11 - R11
    12 - R12
    13 - R13
    14 - R14
    15 - R15
*/

#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)

/* Last register + 1. */
#define TMP_REG1	(SLJIT_NO_REGISTERS + 1)

static SLJIT_CONST sljit_ub reg_map[SLJIT_NO_REGISTERS + 2] = {
	0, 0, 2, 1, 0, 0, 3, 6, 7, 0, 0, 4, 5
};

#define CHECK_EXTRA_REGS(p, w, do) \
	if (p >= SLJIT_TEMPORARY_EREG1 && p <= SLJIT_TEMPORARY_EREG2) { \
		w = compiler->scratches_start + (p - SLJIT_TEMPORARY_EREG1) * sizeof(sljit_sw); \
		p = SLJIT_MEM1(SLJIT_LOCALS_REG); \
		do; \
	} \
	else if (p >= SLJIT_SAVED_EREG1 && p <= SLJIT_SAVED_EREG2) { \
		w = compiler->saveds_start + (p - SLJIT_SAVED_EREG1) * sizeof(sljit_sw); \
		p = SLJIT_MEM1(SLJIT_LOCALS_REG); \
		do; \
	}

#else /* SLJIT_CONFIG_X86_32 */

/* Last register + 1. */
#define TMP_REG1	(SLJIT_NO_REGISTERS + 1)
#define TMP_REG2	(SLJIT_NO_REGISTERS + 2)
#define TMP_REG3	(SLJIT_NO_REGISTERS + 3)

/* Note: r12 & 0x7 == 0b100, which decoded as SIB byte present
   Note: avoid to use r12 and r13 for memory addessing
   therefore r12 is better for SAVED_EREG than SAVED_REG. */
#ifndef _WIN64
/* 1st passed in rdi, 2nd argument passed in rsi, 3rd in rdx. */
static SLJIT_CONST sljit_ub reg_map[SLJIT_NO_REGISTERS + 4] = {
	0, 0, 6, 1, 8, 11, 3, 15, 14, 13, 12, 4, 2, 7, 9
};
/* low-map. reg_map & 0x7. */
static SLJIT_CONST sljit_ub reg_lmap[SLJIT_NO_REGISTERS + 4] = {
	0, 0, 6, 1, 0, 3,  3, 7,  6,  5,  4,  4, 2, 7, 1
};
#else
/* 1st passed in rcx, 2nd argument passed in rdx, 3rd in r8. */
static SLJIT_CONST sljit_ub reg_map[SLJIT_NO_REGISTERS + 4] = {
	0, 0, 2, 1, 11, 13, 3, 6, 7, 14, 15, 4, 10, 8, 9
};
/* low-map. reg_map & 0x7. */
static SLJIT_CONST sljit_ub reg_lmap[SLJIT_NO_REGISTERS + 4] = {
	0, 0, 2, 1, 3,  5,  3, 6, 7,  6,  7, 4, 2,  0, 1
};
#endif

#define REX_W		0x48
#define REX_R		0x44
#define REX_X		0x42
#define REX_B		0x41
#define REX		0x40

#ifndef _WIN64
#define HALFWORD_MAX 0x7fffffffl
#define HALFWORD_MIN -0x80000000l
#else
#define HALFWORD_MAX 0x7fffffffll
#define HALFWORD_MIN -0x80000000ll
#endif

#define IS_HALFWORD(x)		((x) <= HALFWORD_MAX && (x) >= HALFWORD_MIN)
#define NOT_HALFWORD(x)		((x) > HALFWORD_MAX || (x) < HALFWORD_MIN)

#define CHECK_EXTRA_REGS(p, w, do)

#endif /* SLJIT_CONFIG_X86_32 */

#if (defined SLJIT_SSE2 && SLJIT_SSE2)
#define TMP_FREG	(0)
#endif

/* Size flags for emit_x86_instruction: */
#define EX86_BIN_INS		0x0010
#define EX86_SHIFT_INS		0x0020
#define EX86_REX		0x0040
#define EX86_NO_REXW		0x0080
#define EX86_BYTE_ARG		0x0100
#define EX86_HALF_ARG		0x0200
#define EX86_PREF_66		0x0400

#if (defined SLJIT_SSE2 && SLJIT_SSE2)
#define EX86_SSE2		0x0800
#define EX86_PREF_F2		0x1000
#define EX86_PREF_F3		0x2000
#endif

/* --------------------------------------------------------------------- */
/*  Instrucion forms                                                     */
/* --------------------------------------------------------------------- */

#define ADD		(/* BINARY */ 0 << 3)
#define ADD_EAX_i32	0x05
#define ADD_r_rm	0x03
#define ADD_rm_r	0x01
#define ADDSD_x_xm	0x58
#define ADC		(/* BINARY */ 2 << 3)
#define ADC_EAX_i32	0x15
#define ADC_r_rm	0x13
#define ADC_rm_r	0x11
#define AND		(/* BINARY */ 4 << 3)
#define AND_EAX_i32	0x25
#define AND_r_rm	0x23
#define AND_rm_r	0x21
#define ANDPD_x_xm	0x54
#define BSR_r_rm	(/* GROUP_0F */ 0xbd)
#define CALL_i32	0xe8
#define CALL_rm		(/* GROUP_FF */ 2 << 3)
#define CDQ		0x99
#define CMOVNE_r_rm	(/* GROUP_0F */ 0x45)
#define CMP		(/* BINARY */ 7 << 3)
#define CMP_EAX_i32	0x3d
#define CMP_r_rm	0x3b
#define CMP_rm_r	0x39
#define DIV		(/* GROUP_F7 */ 6 << 3)
#define DIVSD_x_xm	0x5e
#define INT3		0xcc
#define IDIV		(/* GROUP_F7 */ 7 << 3)
#define IMUL		(/* GROUP_F7 */ 5 << 3)
#define IMUL_r_rm	(/* GROUP_0F */ 0xaf)
#define IMUL_r_rm_i8	0x6b
#define IMUL_r_rm_i32	0x69
#define JE_i8		0x74
#define JMP_i8		0xeb
#define JMP_i32		0xe9
#define JMP_rm		(/* GROUP_FF */ 4 << 3)
#define LEA_r_m		0x8d
#define MOV_r_rm	0x8b
#define MOV_r_i32	0xb8
#define MOV_rm_r	0x89
#define MOV_rm_i32	0xc7
#define MOV_rm8_i8	0xc6
#define MOV_rm8_r8	0x88
#define MOVSD_x_xm	0x10
#define MOVSD_xm_x	0x11
#define MOVSXD_r_rm	0x63
#define MOVSX_r_rm8	(/* GROUP_0F */ 0xbe)
#define MOVSX_r_rm16	(/* GROUP_0F */ 0xbf)
#define MOVZX_r_rm8	(/* GROUP_0F */ 0xb6)
#define MOVZX_r_rm16	(/* GROUP_0F */ 0xb7)
#define MUL		(/* GROUP_F7 */ 4 << 3)
#define MULSD_x_xm	0x59
#define NEG_rm		(/* GROUP_F7 */ 3 << 3)
#define NOP		0x90
#define NOT_rm		(/* GROUP_F7 */ 2 << 3)
#define OR		(/* BINARY */ 1 << 3)
#define OR_r_rm		0x0b
#define OR_EAX_i32	0x0d
#define OR_rm_r		0x09
#define OR_rm8_r8	0x08
#define POP_r		0x58
#define POP_rm		0x8f
#define POPF		0x9d
#define PUSH_i32	0x68
#define PUSH_r		0x50
#define PUSH_rm		(/* GROUP_FF */ 6 << 3)
#define PUSHF		0x9c
#define RET_near	0xc3
#define RET_i16		0xc2
#define SBB		(/* BINARY */ 3 << 3)
#define SBB_EAX_i32	0x1d
#define SBB_r_rm	0x1b
#define SBB_rm_r	0x19
#define SAR		(/* SHIFT */ 7 << 3)
#define SHL		(/* SHIFT */ 4 << 3)
#define SHR		(/* SHIFT */ 5 << 3)
#define SUB		(/* BINARY */ 5 << 3)
#define SUB_EAX_i32	0x2d
#define SUB_r_rm	0x2b
#define SUB_rm_r	0x29
#define SUBSD_x_xm	0x5c
#define TEST_EAX_i32	0xa9
#define TEST_rm_r	0x85
#define UCOMISD_x_xm	0x2e
#define XCHG_EAX_r	0x90
#define XCHG_r_rm	0x87
#define XOR		(/* BINARY */ 6 << 3)
#define XOR_EAX_i32	0x35
#define XOR_r_rm	0x33
#define XOR_rm_r	0x31
#define XORPD_x_xm	0x57

#define GROUP_0F	0x0f
#define GROUP_F7	0xf7
#define GROUP_FF	0xff
#define GROUP_BINARY_81	0x81
#define GROUP_BINARY_83	0x83
#define GROUP_SHIFT_1	0xd1
#define GROUP_SHIFT_N	0xc1
#define GROUP_SHIFT_CL	0xd3

#define MOD_REG		0xc0
#define MOD_DISP8	0x40

#define INC_SIZE(s)			(*inst++ = (s), compiler->size += (s))

#define PUSH_REG(r)			(*inst++ = (PUSH_r + (r)))
#define POP_REG(r)			(*inst++ = (POP_r + (r)))
#define RET()				(*inst++ = (RET_near))
#define RET_I16(n)			(*inst++ = (RET_i16), *inst++ = n, *inst++ = 0)
/* r32, r/m32 */
#define MOV_RM(mod, reg, rm)		(*inst++ = (MOV_r_rm), *inst++ = (mod) << 6 | (reg) << 3 | (rm))

/* Multithreading does not affect these static variables, since they store
   built-in CPU features. Therefore they can be overwritten by different threads
   if they detect the CPU features in the same time. */
#if (defined SLJIT_SSE2 && SLJIT_SSE2) && (defined SLJIT_DETECT_SSE2 && SLJIT_DETECT_SSE2)
static sljit_si cpu_has_sse2 = -1;
#endif
static sljit_si cpu_has_cmov = -1;

#ifdef _WIN32_WCE
#include <cmnintrin.h>
#elif defined(_MSC_VER) && _MSC_VER >= 1400
#include <intrin.h>
#endif

static void get_cpu_features(void)
{
	sljit_ui features;

#if defined(_MSC_VER) && _MSC_VER >= 1400

	int CPUInfo[4];
	__cpuid(CPUInfo, 1);
	features = (sljit_ui)CPUInfo[3];

#elif defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__SUNPRO_C)

	/* AT&T syntax. */
	__asm__ (
		"movl $0x1, %%eax\n"
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
		/* On x86-32, there is no red zone, so this
		   should work (no need for a local variable). */
		"push %%ebx\n"
#endif
		"cpuid\n"
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
		"pop %%ebx\n"
#endif
		"movl %%edx, %0\n"
		: "=g" (features)
		:
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
		: "%eax", "%ecx", "%edx"
#else
		: "%rax", "%rbx", "%rcx", "%rdx"
#endif
	);

#else /* _MSC_VER && _MSC_VER >= 1400 */

	/* Intel syntax. */
	__asm {
		mov eax, 1
		cpuid
		mov features, edx
	}

#endif /* _MSC_VER && _MSC_VER >= 1400 */

#if (defined SLJIT_SSE2 && SLJIT_SSE2) && (defined SLJIT_DETECT_SSE2 && SLJIT_DETECT_SSE2)
	cpu_has_sse2 = (features >> 26) & 0x1;
#endif
	cpu_has_cmov = (features >> 15) & 0x1;
}

static sljit_ub get_jump_code(sljit_si type)
{
	switch (type) {
	case SLJIT_C_EQUAL:
	case SLJIT_C_FLOAT_EQUAL:
		return 0x84 /* je */;

	case SLJIT_C_NOT_EQUAL:
	case SLJIT_C_FLOAT_NOT_EQUAL:
		return 0x85 /* jne */;

	case SLJIT_C_LESS:
	case SLJIT_C_FLOAT_LESS:
		return 0x82 /* jc */;

	case SLJIT_C_GREATER_EQUAL:
	case SLJIT_C_FLOAT_GREATER_EQUAL:
		return 0x83 /* jae */;

	case SLJIT_C_GREATER:
	case SLJIT_C_FLOAT_GREATER:
		return 0x87 /* jnbe */;

	case SLJIT_C_LESS_EQUAL:
	case SLJIT_C_FLOAT_LESS_EQUAL:
		return 0x86 /* jbe */;

	case SLJIT_C_SIG_LESS:
		return 0x8c /* jl */;

	case SLJIT_C_SIG_GREATER_EQUAL:
		return 0x8d /* jnl */;

	case SLJIT_C_SIG_GREATER:
		return 0x8f /* jnle */;

	case SLJIT_C_SIG_LESS_EQUAL:
		return 0x8e /* jle */;

	case SLJIT_C_OVERFLOW:
	case SLJIT_C_MUL_OVERFLOW:
		return 0x80 /* jo */;

	case SLJIT_C_NOT_OVERFLOW:
	case SLJIT_C_MUL_NOT_OVERFLOW:
		return 0x81 /* jno */;

	case SLJIT_C_FLOAT_UNORDERED:
		return 0x8a /* jp */;

	case SLJIT_C_FLOAT_ORDERED:
		return 0x8b /* jpo */;
	}
	return 0;
}

static sljit_ub* generate_far_jump_code(struct sljit_jump *jump, sljit_ub *code_ptr, sljit_si type);

#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
static sljit_ub* generate_fixed_jump(sljit_ub *code_ptr, sljit_sw addr, sljit_si type);
#endif

static sljit_ub* generate_near_jump_code(struct sljit_jump *jump, sljit_ub *code_ptr, sljit_ub *code, sljit_si type)
{
	sljit_si short_jump;
	sljit_uw label_addr;

	if (jump->flags & JUMP_LABEL)
		label_addr = (sljit_uw)(code + jump->u.label->size);
	else
		label_addr = jump->u.target;
	short_jump = (sljit_sw)(label_addr - (jump->addr + 2)) >= -128 && (sljit_sw)(label_addr - (jump->addr + 2)) <= 127;

#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
	if ((sljit_sw)(label_addr - (jump->addr + 1)) > HALFWORD_MAX || (sljit_sw)(label_addr - (jump->addr + 1)) < HALFWORD_MIN)
		return generate_far_jump_code(jump, code_ptr, type);
#endif

	if (type == SLJIT_JUMP) {
		if (short_jump)
			*code_ptr++ = JMP_i8;
		else
			*code_ptr++ = JMP_i32;
		jump->addr++;
	}
	else if (type >= SLJIT_FAST_CALL) {
		short_jump = 0;
		*code_ptr++ = CALL_i32;
		jump->addr++;
	}
	else if (short_jump) {
		*code_ptr++ = get_jump_code(type) - 0x10;
		jump->addr++;
	}
	else {
		*code_ptr++ = GROUP_0F;
		*code_ptr++ = get_jump_code(type);
		jump->addr += 2;
	}

	if (short_jump) {
		jump->flags |= PATCH_MB;
		code_ptr += sizeof(sljit_sb);
	} else {
		jump->flags |= PATCH_MW;
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
		code_ptr += sizeof(sljit_sw);
#else
		code_ptr += sizeof(sljit_si);
#endif
	}

	return code_ptr;
}

SLJIT_API_FUNC_ATTRIBUTE void* sljit_generate_code(struct sljit_compiler *compiler)
{
	struct sljit_memory_fragment *buf;
	sljit_ub *code;
	sljit_ub *code_ptr;
	sljit_ub *buf_ptr;
	sljit_ub *buf_end;
	sljit_ub len;

	struct sljit_label *label;
	struct sljit_jump *jump;
	struct sljit_const *const_;

	CHECK_ERROR_PTR();
	check_sljit_generate_code(compiler);
	reverse_buf(compiler);

	/* Second code generation pass. */
	code = (sljit_ub*)SLJIT_MALLOC_EXEC(compiler->size);
	PTR_FAIL_WITH_EXEC_IF(code);
	buf = compiler->buf;

	code_ptr = code;
	label = compiler->labels;
	jump = compiler->jumps;
	const_ = compiler->consts;
	do {
		buf_ptr = buf->memory;
		buf_end = buf_ptr + buf->used_size;
		do {
			len = *buf_ptr++;
			if (len > 0) {
				/* The code is already generated. */
				SLJIT_MEMMOVE(code_ptr, buf_ptr, len);
				code_ptr += len;
				buf_ptr += len;
			}
			else {
				if (*buf_ptr >= 4) {
					jump->addr = (sljit_uw)code_ptr;
					if (!(jump->flags & SLJIT_REWRITABLE_JUMP))
						code_ptr = generate_near_jump_code(jump, code_ptr, code, *buf_ptr - 4);
					else
						code_ptr = generate_far_jump_code(jump, code_ptr, *buf_ptr - 4);
					jump = jump->next;
				}
				else if (*buf_ptr == 0) {
					label->addr = (sljit_uw)code_ptr;
					label->size = code_ptr - code;
					label = label->next;
				}
				else if (*buf_ptr == 1) {
					const_->addr = ((sljit_uw)code_ptr) - sizeof(sljit_sw);
					const_ = const_->next;
				}
				else {
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
					*code_ptr++ = (*buf_ptr == 2) ? CALL_i32 : JMP_i32;
					buf_ptr++;
					*(sljit_sw*)code_ptr = *(sljit_sw*)buf_ptr - ((sljit_sw)code_ptr + sizeof(sljit_sw));
					code_ptr += sizeof(sljit_sw);
					buf_ptr += sizeof(sljit_sw) - 1;
#else
					code_ptr = generate_fixed_jump(code_ptr, *(sljit_sw*)(buf_ptr + 1), *buf_ptr);
					buf_ptr += sizeof(sljit_sw);
#endif
				}
				buf_ptr++;
			}
		} while (buf_ptr < buf_end);
		SLJIT_ASSERT(buf_ptr == buf_end);
		buf = buf->next;
	} while (buf);

	SLJIT_ASSERT(!label);
	SLJIT_ASSERT(!jump);
	SLJIT_ASSERT(!const_);

	jump = compiler->jumps;
	while (jump) {
		if (jump->flags & PATCH_MB) {
			SLJIT_ASSERT((sljit_sw)(jump->u.label->addr - (jump->addr + sizeof(sljit_sb))) >= -128 && (sljit_sw)(jump->u.label->addr - (jump->addr + sizeof(sljit_sb))) <= 127);
			*(sljit_ub*)jump->addr = (sljit_ub)(jump->u.label->addr - (jump->addr + sizeof(sljit_sb)));
		} else if (jump->flags & PATCH_MW) {
			if (jump->flags & JUMP_LABEL) {
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
				*(sljit_sw*)jump->addr = (sljit_sw)(jump->u.label->addr - (jump->addr + sizeof(sljit_sw)));
#else
				SLJIT_ASSERT((sljit_sw)(jump->u.label->addr - (jump->addr + sizeof(sljit_si))) >= HALFWORD_MIN && (sljit_sw)(jump->u.label->addr - (jump->addr + sizeof(sljit_si))) <= HALFWORD_MAX);
				*(sljit_si*)jump->addr = (sljit_si)(jump->u.label->addr - (jump->addr + sizeof(sljit_si)));
#endif
			}
			else {
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
				*(sljit_sw*)jump->addr = (sljit_sw)(jump->u.target - (jump->addr + sizeof(sljit_sw)));
#else
				SLJIT_ASSERT((sljit_sw)(jump->u.target - (jump->addr + sizeof(sljit_si))) >= HALFWORD_MIN && (sljit_sw)(jump->u.target - (jump->addr + sizeof(sljit_si))) <= HALFWORD_MAX);
				*(sljit_si*)jump->addr = (sljit_si)(jump->u.target - (jump->addr + sizeof(sljit_si)));
#endif
			}
		}
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
		else if (jump->flags & PATCH_MD)
			*(sljit_sw*)jump->addr = jump->u.label->addr;
#endif

		jump = jump->next;
	}

	/* Maybe we waste some space because of short jumps. */
	SLJIT_ASSERT(code_ptr <= code + compiler->size);
	compiler->error = SLJIT_ERR_COMPILED;
	compiler->executable_size = code_ptr - code;
	return (void*)code;
}

/* --------------------------------------------------------------------- */
/*  Operators                                                            */
/* --------------------------------------------------------------------- */

static sljit_si emit_cum_binary(struct sljit_compiler *compiler,
	sljit_ub op_rm, sljit_ub op_mr, sljit_ub op_imm, sljit_ub op_eax_imm,
	sljit_si dst, sljit_sw dstw,
	sljit_si src1, sljit_sw src1w,
	sljit_si src2, sljit_sw src2w);

static sljit_si emit_non_cum_binary(struct sljit_compiler *compiler,
	sljit_ub op_rm, sljit_ub op_mr, sljit_ub op_imm, sljit_ub op_eax_imm,
	sljit_si dst, sljit_sw dstw,
	sljit_si src1, sljit_sw src1w,
	sljit_si src2, sljit_sw src2w);

static sljit_si emit_mov(struct sljit_compiler *compiler,
	sljit_si dst, sljit_sw dstw,
	sljit_si src, sljit_sw srcw);

static SLJIT_INLINE sljit_si emit_save_flags(struct sljit_compiler *compiler)
{
	sljit_ub *inst;

#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
	inst = (sljit_ub*)ensure_buf(compiler, 1 + 5);
	FAIL_IF(!inst);
	INC_SIZE(5);
#else
	inst = (sljit_ub*)ensure_buf(compiler, 1 + 6);
	FAIL_IF(!inst);
	INC_SIZE(6);
	*inst++ = REX_W;
#endif
	*inst++ = LEA_r_m; /* lea esp/rsp, [esp/rsp + sizeof(sljit_sw)] */
	*inst++ = 0x64;
	*inst++ = 0x24;
	*inst++ = (sljit_ub)sizeof(sljit_sw);
	*inst++ = PUSHF;
	compiler->flags_saved = 1;
	return SLJIT_SUCCESS;
}

static SLJIT_INLINE sljit_si emit_restore_flags(struct sljit_compiler *compiler, sljit_si keep_flags)
{
	sljit_ub *inst;

#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
	inst = (sljit_ub*)ensure_buf(compiler, 1 + 5);
	FAIL_IF(!inst);
	INC_SIZE(5);
	*inst++ = POPF;
#else
	inst = (sljit_ub*)ensure_buf(compiler, 1 + 6);
	FAIL_IF(!inst);
	INC_SIZE(6);
	*inst++ = POPF;
	*inst++ = REX_W;
#endif
	*inst++ = LEA_r_m; /* lea esp/rsp, [esp/rsp - sizeof(sljit_sw)] */
	*inst++ = 0x64;
	*inst++ = 0x24;
	*inst++ = (sljit_ub)-(sljit_sb)sizeof(sljit_sw);
	compiler->flags_saved = keep_flags;
	return SLJIT_SUCCESS;
}

#ifdef _WIN32
#include <malloc.h>

static void SLJIT_CALL sljit_grow_stack(sljit_sw local_size)
{
	/* Workaround for calling the internal _chkstk() function on Windows.
	This function touches all 4k pages belongs to the requested stack space,
	which size is passed in local_size. This is necessary on Windows where
	the stack can only grow in 4k steps. However, this function just burn
	CPU cycles if the stack is large enough. However, you don't know it in
	advance, so it must always be called. I think this is a bad design in
	general even if it has some reasons. */
	*(volatile sljit_si*)alloca(local_size) = 0;
}

#endif

#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
#include "sljitNativeX86_32.c"
#else
#include "sljitNativeX86_64.c"
#endif

static sljit_si emit_mov(struct sljit_compiler *compiler,
	sljit_si dst, sljit_sw dstw,
	sljit_si src, sljit_sw srcw)
{
	sljit_ub* inst;

	if (dst == SLJIT_UNUSED) {
		/* No destination, doesn't need to setup flags. */
		if (src & SLJIT_MEM) {
			inst = emit_x86_instruction(compiler, 1, TMP_REG1, 0, src, srcw);
			FAIL_IF(!inst);
			*inst = MOV_r_rm;
		}
		return SLJIT_SUCCESS;
	}
	if (FAST_IS_REG(src)) {
		inst = emit_x86_instruction(compiler, 1, src, 0, dst, dstw);
		FAIL_IF(!inst);
		*inst = MOV_rm_r;
		return SLJIT_SUCCESS;
	}
	if (src & SLJIT_IMM) {
		if (FAST_IS_REG(dst)) {
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
			return emit_do_imm(compiler, MOV_r_i32 + reg_map[dst], srcw);
#else
			if (!compiler->mode32) {
				if (NOT_HALFWORD(srcw))
					return emit_load_imm64(compiler, dst, srcw);
			}
			else
				return emit_do_imm32(compiler, (reg_map[dst] >= 8) ? REX_B : 0, MOV_r_i32 + reg_lmap[dst], srcw);
#endif
		}
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
		if (!compiler->mode32 && NOT_HALFWORD(srcw)) {
			FAIL_IF(emit_load_imm64(compiler, TMP_REG2, srcw));
			inst = emit_x86_instruction(compiler, 1, TMP_REG2, 0, dst, dstw);
			FAIL_IF(!inst);
			*inst = MOV_rm_r;
			return SLJIT_SUCCESS;
		}
#endif
		inst = emit_x86_instruction(compiler, 1, SLJIT_IMM, srcw, dst, dstw);
		FAIL_IF(!inst);
		*inst = MOV_rm_i32;
		return SLJIT_SUCCESS;
	}
	if (FAST_IS_REG(dst)) {
		inst = emit_x86_instruction(compiler, 1, dst, 0, src, srcw);
		FAIL_IF(!inst);
		*inst = MOV_r_rm;
		return SLJIT_SUCCESS;
	}

	/* Memory to memory move. Requires two instruction. */
	inst = emit_x86_instruction(compiler, 1, TMP_REG1, 0, src, srcw);
	FAIL_IF(!inst);
	*inst = MOV_r_rm;
	inst = emit_x86_instruction(compiler, 1, TMP_REG1, 0, dst, dstw);
	FAIL_IF(!inst);
	*inst = MOV_rm_r;
	return SLJIT_SUCCESS;
}

#define EMIT_MOV(compiler, dst, dstw, src, srcw) \
	FAIL_IF(emit_mov(compiler, dst, dstw, src, srcw));

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_emit_op0(struct sljit_compiler *compiler, sljit_si op)
{
	sljit_ub *inst;
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
	sljit_si size;
#endif

	CHECK_ERROR();
	check_sljit_emit_op0(compiler, op);

	switch (GET_OPCODE(op)) {
	case SLJIT_BREAKPOINT:
		inst = (sljit_ub*)ensure_buf(compiler, 1 + 1);
		FAIL_IF(!inst);
		INC_SIZE(1);
		*inst = INT3;
		break;
	case SLJIT_NOP:
		inst = (sljit_ub*)ensure_buf(compiler, 1 + 1);
		FAIL_IF(!inst);
		INC_SIZE(1);
		*inst = NOP;
		break;
	case SLJIT_UMUL:
	case SLJIT_SMUL:
	case SLJIT_UDIV:
	case SLJIT_SDIV:
		compiler->flags_saved = 0;
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
#ifdef _WIN64
		SLJIT_COMPILE_ASSERT(
			reg_map[SLJIT_SCRATCH_REG1] == 0
			&& reg_map[SLJIT_SCRATCH_REG2] == 2
			&& reg_map[TMP_REG1] > 7,
			invalid_register_assignment_for_div_mul);
#else
		SLJIT_COMPILE_ASSERT(
			reg_map[SLJIT_SCRATCH_REG1] == 0
			&& reg_map[SLJIT_SCRATCH_REG2] < 7
			&& reg_map[TMP_REG1] == 2,
			invalid_register_assignment_for_div_mul);
#endif
		compiler->mode32 = op & SLJIT_INT_OP;
#endif

		op = GET_OPCODE(op);
		if (op == SLJIT_UDIV) {
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32) || defined(_WIN64)
			EMIT_MOV(compiler, TMP_REG1, 0, SLJIT_SCRATCH_REG2, 0);
			inst = emit_x86_instruction(compiler, 1, SLJIT_SCRATCH_REG2, 0, SLJIT_SCRATCH_REG2, 0);
#else
			inst = emit_x86_instruction(compiler, 1, TMP_REG1, 0, TMP_REG1, 0);
#endif
			FAIL_IF(!inst);
			*inst = XOR_r_rm;
		}

		if (op == SLJIT_SDIV) {
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32) || defined(_WIN64)
			EMIT_MOV(compiler, TMP_REG1, 0, SLJIT_SCRATCH_REG2, 0);
#endif

#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
			inst = (sljit_ub*)ensure_buf(compiler, 1 + 1);
			FAIL_IF(!inst);
			INC_SIZE(1);
			*inst = CDQ;
#else
			if (compiler->mode32) {
				inst = (sljit_ub*)ensure_buf(compiler, 1 + 1);
				FAIL_IF(!inst);
				INC_SIZE(1);
				*inst = CDQ;
			} else {
				inst = (sljit_ub*)ensure_buf(compiler, 1 + 2);
				FAIL_IF(!inst);
				INC_SIZE(2);
				*inst++ = REX_W;
				*inst = CDQ;
			}
#endif
		}

#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
		inst = (sljit_ub*)ensure_buf(compiler, 1 + 2);
		FAIL_IF(!inst);
		INC_SIZE(2);
		*inst++ = GROUP_F7;
		*inst = MOD_REG | ((op >= SLJIT_UDIV) ? reg_map[TMP_REG1] : reg_map[SLJIT_SCRATCH_REG2]);
#else
#ifdef _WIN64
		size = (!compiler->mode32 || op >= SLJIT_UDIV) ? 3 : 2;
#else
		size = (!compiler->mode32) ? 3 : 2;
#endif
		inst = (sljit_ub*)ensure_buf(compiler, 1 + size);
		FAIL_IF(!inst);
		INC_SIZE(size);
#ifdef _WIN64
		if (!compiler->mode32)
			*inst++ = REX_W | ((op >= SLJIT_UDIV) ? REX_B : 0);
		else if (op >= SLJIT_UDIV)
			*inst++ = REX_B;
		*inst++ = GROUP_F7;
		*inst = MOD_REG | ((op >= SLJIT_UDIV) ? reg_lmap[TMP_REG1] : reg_lmap[SLJIT_SCRATCH_REG2]);
#else
		if (!compiler->mode32)
			*inst++ = REX_W;
		*inst++ = GROUP_F7;
		*inst = MOD_REG | reg_map[SLJIT_SCRATCH_REG2];
#endif
#endif
		switch (op) {
		case SLJIT_UMUL:
			*inst |= MUL;
			break;
		case SLJIT_SMUL:
			*inst |= IMUL;
			break;
		case SLJIT_UDIV:
			*inst |= DIV;
			break;
		case SLJIT_SDIV:
			*inst |= IDIV;
			break;
		}
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64) && !defined(_WIN64)
		EMIT_MOV(compiler, SLJIT_SCRATCH_REG2, 0, TMP_REG1, 0);
#endif
		break;
	}

	return SLJIT_SUCCESS;
}

#define ENCODE_PREFIX(prefix) \
	do { \
		inst = (sljit_ub*)ensure_buf(compiler, 1 + 1); \
		FAIL_IF(!inst); \
		INC_SIZE(1); \
		*inst = (prefix); \
	} while (0)

static sljit_si emit_mov_byte(struct sljit_compiler *compiler, sljit_si sign,
	sljit_si dst, sljit_sw dstw,
	sljit_si src, sljit_sw srcw)
{
	sljit_ub* inst;
	sljit_si dst_r;
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
	sljit_si work_r;
#endif

#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
	compiler->mode32 = 0;
#endif

	if (dst == SLJIT_UNUSED && !(src & SLJIT_MEM))
		return SLJIT_SUCCESS; /* Empty instruction. */

	if (src & SLJIT_IMM) {
		if (FAST_IS_REG(dst)) {
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
			return emit_do_imm(compiler, MOV_r_i32 + reg_map[dst], srcw);
#else
			inst = emit_x86_instruction(compiler, 1, SLJIT_IMM, srcw, dst, 0);
			FAIL_IF(!inst);
			*inst = MOV_rm_i32;
			return SLJIT_SUCCESS;
#endif
		}
		inst = emit_x86_instruction(compiler, 1 | EX86_BYTE_ARG | EX86_NO_REXW, SLJIT_IMM, srcw, dst, dstw);
		FAIL_IF(!inst);
		*inst = MOV_rm8_i8;
		return SLJIT_SUCCESS;
	}

	dst_r = FAST_IS_REG(dst) ? dst : TMP_REG1;

	if ((dst & SLJIT_MEM) && FAST_IS_REG(src)) {
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
		if (reg_map[src] >= 4) {
			SLJIT_ASSERT(dst_r == TMP_REG1);
			EMIT_MOV(compiler, TMP_REG1, 0, src, 0);
		} else
			dst_r = src;
#else
		dst_r = src;
#endif
	}
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
	else if (FAST_IS_REG(src) && reg_map[src] >= 4) {
		/* src, dst are registers. */
		SLJIT_ASSERT(SLOW_IS_REG(dst));
		if (reg_map[dst] < 4) {
			if (dst != src)
				EMIT_MOV(compiler, dst, 0, src, 0);
			inst = emit_x86_instruction(compiler, 2, dst, 0, dst, 0);
			FAIL_IF(!inst);
			*inst++ = GROUP_0F;
			*inst = sign ? MOVSX_r_rm8 : MOVZX_r_rm8;
		}
		else {
			if (dst != src)
				EMIT_MOV(compiler, dst, 0, src, 0);
			if (sign) {
				/* shl reg, 24 */
				inst = emit_x86_instruction(compiler, 1 | EX86_SHIFT_INS, SLJIT_IMM, 24, dst, 0);
				FAIL_IF(!inst);
				*inst |= SHL;
				/* sar reg, 24 */
				inst = emit_x86_instruction(compiler, 1 | EX86_SHIFT_INS, SLJIT_IMM, 24, dst, 0);
				FAIL_IF(!inst);
				*inst |= SAR;
			}
			else {
				inst = emit_x86_instruction(compiler, 1 | EX86_BIN_INS, SLJIT_IMM, 0xff, dst, 0);
				FAIL_IF(!inst);
				*(inst + 1) |= AND;
			}
		}
		return SLJIT_SUCCESS;
	}
#endif
	else {
		/* src can be memory addr or reg_map[src] < 4 on x86_32 architectures. */
		inst = emit_x86_instruction(compiler, 2, dst_r, 0, src, srcw);
		FAIL_IF(!inst);
		*inst++ = GROUP_0F;
		*inst = sign ? MOVSX_r_rm8 : MOVZX_r_rm8;
	}

	if (dst & SLJIT_MEM) {
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
		if (dst_r == TMP_REG1) {
			/* Find a non-used register, whose reg_map[src] < 4. */
			if ((dst & REG_MASK) == SLJIT_SCRATCH_REG1) {
				if ((dst & OFFS_REG_MASK) == TO_OFFS_REG(SLJIT_SCRATCH_REG2))
					work_r = SLJIT_SCRATCH_REG3;
				else
					work_r = SLJIT_SCRATCH_REG2;
			}
			else {
				if ((dst & OFFS_REG_MASK) != TO_OFFS_REG(SLJIT_SCRATCH_REG1))
					work_r = SLJIT_SCRATCH_REG1;
				else if ((dst & REG_MASK) == SLJIT_SCRATCH_REG2)
					work_r = SLJIT_SCRATCH_REG3;
				else
					work_r = SLJIT_SCRATCH_REG2;
			}

			if (work_r == SLJIT_SCRATCH_REG1) {
				ENCODE_PREFIX(XCHG_EAX_r + reg_map[TMP_REG1]);
			}
			else {
				inst = emit_x86_instruction(compiler, 1, work_r, 0, dst_r, 0);
				FAIL_IF(!inst);
				*inst = XCHG_r_rm;
			}

			inst = emit_x86_instruction(compiler, 1, work_r, 0, dst, dstw);
			FAIL_IF(!inst);
			*inst = MOV_rm8_r8;

			if (work_r == SLJIT_SCRATCH_REG1) {
				ENCODE_PREFIX(XCHG_EAX_r + reg_map[TMP_REG1]);
			}
			else {
				inst = emit_x86_instruction(compiler, 1, work_r, 0, dst_r, 0);
				FAIL_IF(!inst);
				*inst = XCHG_r_rm;
			}
		}
		else {
			inst = emit_x86_instruction(compiler, 1, dst_r, 0, dst, dstw);
			FAIL_IF(!inst);
			*inst = MOV_rm8_r8;
		}
#else
		inst = emit_x86_instruction(compiler, 1 | EX86_REX | EX86_NO_REXW, dst_r, 0, dst, dstw);
		FAIL_IF(!inst);
		*inst = MOV_rm8_r8;
#endif
	}

	return SLJIT_SUCCESS;
}

static sljit_si emit_mov_half(struct sljit_compiler *compiler, sljit_si sign,
	sljit_si dst, sljit_sw dstw,
	sljit_si src, sljit_sw srcw)
{
	sljit_ub* inst;
	sljit_si dst_r;

#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
	compiler->mode32 = 0;
#endif

	if (dst == SLJIT_UNUSED && !(src & SLJIT_MEM))
		return SLJIT_SUCCESS; /* Empty instruction. */

	if (src & SLJIT_IMM) {
		if (FAST_IS_REG(dst)) {
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
			return emit_do_imm(compiler, MOV_r_i32 + reg_map[dst], srcw);
#else
			inst = emit_x86_instruction(compiler, 1, SLJIT_IMM, srcw, dst, 0);
			FAIL_IF(!inst);
			*inst = MOV_rm_i32;
			return SLJIT_SUCCESS;
#endif
		}
		inst = emit_x86_instruction(compiler, 1 | EX86_HALF_ARG | EX86_NO_REXW | EX86_PREF_66, SLJIT_IMM, srcw, dst, dstw);
		FAIL_IF(!inst);
		*inst = MOV_rm_i32;
		return SLJIT_SUCCESS;
	}

	dst_r = FAST_IS_REG(dst) ? dst : TMP_REG1;

	if ((dst & SLJIT_MEM) && FAST_IS_REG(src))
		dst_r = src;
	else {
		inst = emit_x86_instruction(compiler, 2, dst_r, 0, src, srcw);
		FAIL_IF(!inst);
		*inst++ = GROUP_0F;
		*inst = sign ? MOVSX_r_rm16 : MOVZX_r_rm16;
	}

	if (dst & SLJIT_MEM) {
		inst = emit_x86_instruction(compiler, 1 | EX86_NO_REXW | EX86_PREF_66, dst_r, 0, dst, dstw);
		FAIL_IF(!inst);
		*inst = MOV_rm_r;
	}

	return SLJIT_SUCCESS;
}

static sljit_si emit_unary(struct sljit_compiler *compiler, sljit_ub opcode,
	sljit_si dst, sljit_sw dstw,
	sljit_si src, sljit_sw srcw)
{
	sljit_ub* inst;

	if (dst == SLJIT_UNUSED) {
		EMIT_MOV(compiler, TMP_REG1, 0, src, srcw);
		inst = emit_x86_instruction(compiler, 1, 0, 0, TMP_REG1, 0);
		FAIL_IF(!inst);
		*inst++ = GROUP_F7;
		*inst |= opcode;
		return SLJIT_SUCCESS;
	}
	if (dst == src && dstw == srcw) {
		/* Same input and output */
		inst = emit_x86_instruction(compiler, 1, 0, 0, dst, dstw);
		FAIL_IF(!inst);
		*inst++ = GROUP_F7;
		*inst |= opcode;
		return SLJIT_SUCCESS;
	}
	if (FAST_IS_REG(dst)) {
		EMIT_MOV(compiler, dst, 0, src, srcw);
		inst = emit_x86_instruction(compiler, 1, 0, 0, dst, dstw);
		FAIL_IF(!inst);
		*inst++ = GROUP_F7;
		*inst |= opcode;
		return SLJIT_SUCCESS;
	}
	EMIT_MOV(compiler, TMP_REG1, 0, src, srcw);
	inst = emit_x86_instruction(compiler, 1, 0, 0, TMP_REG1, 0);
	FAIL_IF(!inst);
	*inst++ = GROUP_F7;
	*inst |= opcode;
	EMIT_MOV(compiler, dst, dstw, TMP_REG1, 0);
	return SLJIT_SUCCESS;
}

static sljit_si emit_not_with_flags(struct sljit_compiler *compiler,
	sljit_si dst, sljit_sw dstw,
	sljit_si src, sljit_sw srcw)
{
	sljit_ub* inst;

	if (dst == SLJIT_UNUSED) {
		EMIT_MOV(compiler, TMP_REG1, 0, src, srcw);
		inst = emit_x86_instruction(compiler, 1, 0, 0, TMP_REG1, 0);
		FAIL_IF(!inst);
		*inst++ = GROUP_F7;
		*inst |= NOT_rm;
		inst = emit_x86_instruction(compiler, 1, TMP_REG1, 0, TMP_REG1, 0);
		FAIL_IF(!inst);
		*inst = OR_r_rm;
		return SLJIT_SUCCESS;
	}
	if (FAST_IS_REG(dst)) {
		EMIT_MOV(compiler, dst, 0, src, srcw);
		inst = emit_x86_instruction(compiler, 1, 0, 0, dst, dstw);
		FAIL_IF(!inst);
		*inst++ = GROUP_F7;
		*inst |= NOT_rm;
		inst = emit_x86_instruction(compiler, 1, dst, 0, dst, 0);
		FAIL_IF(!inst);
		*inst = OR_r_rm;
		return SLJIT_SUCCESS;
	}
	EMIT_MOV(compiler, TMP_REG1, 0, src, srcw);
	inst = emit_x86_instruction(compiler, 1, 0, 0, TMP_REG1, 0);
	FAIL_IF(!inst);
	*inst++ = GROUP_F7;
	*inst |= NOT_rm;
	inst = emit_x86_instruction(compiler, 1, TMP_REG1, 0, TMP_REG1, 0);
	FAIL_IF(!inst);
	*inst = OR_r_rm;
	EMIT_MOV(compiler, dst, dstw, TMP_REG1, 0);
	return SLJIT_SUCCESS;
}

static sljit_si emit_clz(struct sljit_compiler *compiler, sljit_si op_flags,
	sljit_si dst, sljit_sw dstw,
	sljit_si src, sljit_sw srcw)
{
	sljit_ub* inst;
	sljit_si dst_r;

	SLJIT_UNUSED_ARG(op_flags);
	if (SLJIT_UNLIKELY(dst == SLJIT_UNUSED)) {
		/* Just set the zero flag. */
		EMIT_MOV(compiler, TMP_REG1, 0, src, srcw);
		inst = emit_x86_instruction(compiler, 1, 0, 0, TMP_REG1, 0);
		FAIL_IF(!inst);
		*inst++ = GROUP_F7;
		*inst |= NOT_rm;
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
		inst = emit_x86_instruction(compiler, 1 | EX86_SHIFT_INS, SLJIT_IMM, 31, TMP_REG1, 0);
#else
		inst = emit_x86_instruction(compiler, 1 | EX86_SHIFT_INS, SLJIT_IMM, !(op_flags & SLJIT_INT_OP) ? 63 : 31, TMP_REG1, 0);
#endif
		FAIL_IF(!inst);
		*inst |= SHR;
		return SLJIT_SUCCESS;
	}

	if (SLJIT_UNLIKELY(src & SLJIT_IMM)) {
		EMIT_MOV(compiler, TMP_REG1, 0, SLJIT_IMM, srcw);
		src = TMP_REG1;
		srcw = 0;
	}

	inst = emit_x86_instruction(compiler, 2, TMP_REG1, 0, src, srcw);
	FAIL_IF(!inst);
	*inst++ = GROUP_0F;
	*inst = BSR_r_rm;

#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
	if (FAST_IS_REG(dst))
		dst_r = dst;
	else {
		/* Find an unused temporary register. */
		if ((dst & REG_MASK) != SLJIT_SCRATCH_REG1 && (dst & OFFS_REG_MASK) != TO_OFFS_REG(SLJIT_SCRATCH_REG1))
			dst_r = SLJIT_SCRATCH_REG1;
		else if ((dst & REG_MASK) != SLJIT_SCRATCH_REG2 && (dst & OFFS_REG_MASK) != TO_OFFS_REG(SLJIT_SCRATCH_REG2))
			dst_r = SLJIT_SCRATCH_REG2;
		else
			dst_r = SLJIT_SCRATCH_REG3;
		EMIT_MOV(compiler, dst, dstw, dst_r, 0);
	}
	EMIT_MOV(compiler, dst_r, 0, SLJIT_IMM, 32 + 31);
#else
	dst_r = FAST_IS_REG(dst) ? dst : TMP_REG2;
	compiler->mode32 = 0;
	EMIT_MOV(compiler, dst_r, 0, SLJIT_IMM, !(op_flags & SLJIT_INT_OP) ? 64 + 63 : 32 + 31);
	compiler->mode32 = op_flags & SLJIT_INT_OP;
#endif

	if (cpu_has_cmov == -1)
		get_cpu_features();

	if (cpu_has_cmov) {
		inst = emit_x86_instruction(compiler, 2, dst_r, 0, TMP_REG1, 0);
		FAIL_IF(!inst);
		*inst++ = GROUP_0F;
		*inst = CMOVNE_r_rm;
	} else {
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
		inst = (sljit_ub*)ensure_buf(compiler, 1 + 4);
		FAIL_IF(!inst);
		INC_SIZE(4);

		*inst++ = JE_i8;
		*inst++ = 2;
		*inst++ = MOV_r_rm;
		*inst++ = MOD_REG | (reg_map[dst_r] << 3) | reg_map[TMP_REG1];
#else
		inst = (sljit_ub*)ensure_buf(compiler, 1 + 5);
		FAIL_IF(!inst);
		INC_SIZE(5);

		*inst++ = JE_i8;
		*inst++ = 3;
		*inst++ = REX_W | (reg_map[dst_r] >= 8 ? REX_R : 0) | (reg_map[TMP_REG1] >= 8 ? REX_B : 0);
		*inst++ = MOV_r_rm;
		*inst++ = MOD_REG | (reg_lmap[dst_r] << 3) | reg_lmap[TMP_REG1];
#endif
	}

#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
	inst = emit_x86_instruction(compiler, 1 | EX86_BIN_INS, SLJIT_IMM, 31, dst_r, 0);
#else
	inst = emit_x86_instruction(compiler, 1 | EX86_BIN_INS, SLJIT_IMM, !(op_flags & SLJIT_INT_OP) ? 63 : 31, dst_r, 0);
#endif
	FAIL_IF(!inst);
	*(inst + 1) |= XOR;

#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
	if (dst & SLJIT_MEM) {
		inst = emit_x86_instruction(compiler, 1, dst_r, 0, dst, dstw);
		FAIL_IF(!inst);
		*inst = XCHG_r_rm;
	}
#else
	if (dst & SLJIT_MEM)
		EMIT_MOV(compiler, dst, dstw, TMP_REG2, 0);
#endif
	return SLJIT_SUCCESS;
}

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_emit_op1(struct sljit_compiler *compiler, sljit_si op,
	sljit_si dst, sljit_sw dstw,
	sljit_si src, sljit_sw srcw)
{
	sljit_ub* inst;
	sljit_si update = 0;
	sljit_si op_flags = GET_ALL_FLAGS(op);
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
	sljit_si dst_is_ereg = 0;
	sljit_si src_is_ereg = 0;
#else
#	define src_is_ereg 0
#endif

	CHECK_ERROR();
	check_sljit_emit_op1(compiler, op, dst, dstw, src, srcw);
	ADJUST_LOCAL_OFFSET(dst, dstw);
	ADJUST_LOCAL_OFFSET(src, srcw);

	CHECK_EXTRA_REGS(dst, dstw, dst_is_ereg = 1);
	CHECK_EXTRA_REGS(src, srcw, src_is_ereg = 1);
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
	compiler->mode32 = op_flags & SLJIT_INT_OP;
#endif

	op = GET_OPCODE(op);
	if (op >= SLJIT_MOV && op <= SLJIT_MOVU_P) {
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
		compiler->mode32 = 0;
#endif

		if (op_flags & SLJIT_INT_OP) {
			if (FAST_IS_REG(src) && src == dst) {
				if (!TYPE_CAST_NEEDED(op))
					return SLJIT_SUCCESS;
			}
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
			if (op == SLJIT_MOV_SI && (src & SLJIT_MEM))
				op = SLJIT_MOV_UI;
			if (op == SLJIT_MOVU_SI && (src & SLJIT_MEM))
				op = SLJIT_MOVU_UI;
			if (op == SLJIT_MOV_UI && (src & SLJIT_IMM))
				op = SLJIT_MOV_SI;
			if (op == SLJIT_MOVU_UI && (src & SLJIT_IMM))
				op = SLJIT_MOVU_SI;
#endif
		}

		SLJIT_COMPILE_ASSERT(SLJIT_MOV + 8 == SLJIT_MOVU, movu_offset);
		if (op >= SLJIT_MOVU) {
			update = 1;
			op -= 8;
		}

		if (src & SLJIT_IMM) {
			switch (op) {
			case SLJIT_MOV_UB:
				srcw = (sljit_ub)srcw;
				break;
			case SLJIT_MOV_SB:
				srcw = (sljit_sb)srcw;
				break;
			case SLJIT_MOV_UH:
				srcw = (sljit_uh)srcw;
				break;
			case SLJIT_MOV_SH:
				srcw = (sljit_sh)srcw;
				break;
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
			case SLJIT_MOV_UI:
				srcw = (sljit_ui)srcw;
				break;
			case SLJIT_MOV_SI:
				srcw = (sljit_si)srcw;
				break;
#endif
			}
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
			if (SLJIT_UNLIKELY(dst_is_ereg))
				return emit_mov(compiler, dst, dstw, src, srcw);
#endif
		}

		if (SLJIT_UNLIKELY(update) && (src & SLJIT_MEM) && !src_is_ereg && (src & REG_MASK) && (srcw != 0 || (src & OFFS_REG_MASK) != 0)) {
			inst = emit_x86_instruction(compiler, 1, src & REG_MASK, 0, src, srcw);
			FAIL_IF(!inst);
			*inst = LEA_r_m;
			src &= SLJIT_MEM | 0xf;
			srcw = 0;
		}

#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
		if (SLJIT_UNLIKELY(dst_is_ereg) && (!(op == SLJIT_MOV || op == SLJIT_MOV_UI || op == SLJIT_MOV_SI || op == SLJIT_MOV_P) || (src & SLJIT_MEM))) {
			SLJIT_ASSERT(dst == SLJIT_MEM1(SLJIT_LOCALS_REG));
			dst = TMP_REG1;
		}
#endif

		switch (op) {
		case SLJIT_MOV:
		case SLJIT_MOV_P:
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
		case SLJIT_MOV_UI:
		case SLJIT_MOV_SI:
#endif
			FAIL_IF(emit_mov(compiler, dst, dstw, src, srcw));
			break;
		case SLJIT_MOV_UB:
			FAIL_IF(emit_mov_byte(compiler, 0, dst, dstw, src, srcw));
			break;
		case SLJIT_MOV_SB:
			FAIL_IF(emit_mov_byte(compiler, 1, dst, dstw, src, srcw));
			break;
		case SLJIT_MOV_UH:
			FAIL_IF(emit_mov_half(compiler, 0, dst, dstw, src, srcw));
			break;
		case SLJIT_MOV_SH:
			FAIL_IF(emit_mov_half(compiler, 1, dst, dstw, src, srcw));
			break;
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
		case SLJIT_MOV_UI:
			FAIL_IF(emit_mov_int(compiler, 0, dst, dstw, src, srcw));
			break;
		case SLJIT_MOV_SI:
			FAIL_IF(emit_mov_int(compiler, 1, dst, dstw, src, srcw));
			break;
#endif
		}

#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
		if (SLJIT_UNLIKELY(dst_is_ereg) && dst == TMP_REG1)
			return emit_mov(compiler, SLJIT_MEM1(SLJIT_LOCALS_REG), dstw, TMP_REG1, 0);
#endif

		if (SLJIT_UNLIKELY(update) && (dst & SLJIT_MEM) && (dst & REG_MASK) && (dstw != 0 || (dst & OFFS_REG_MASK) != 0)) {
			inst = emit_x86_instruction(compiler, 1, dst & REG_MASK, 0, dst, dstw);
			FAIL_IF(!inst);
			*inst = LEA_r_m;
		}
		return SLJIT_SUCCESS;
	}

	if (SLJIT_UNLIKELY(GET_FLAGS(op_flags)))
		compiler->flags_saved = 0;

	switch (op) {
	case SLJIT_NOT:
		if (SLJIT_UNLIKELY(op_flags & SLJIT_SET_E))
			return emit_not_with_flags(compiler, dst, dstw, src, srcw);
		return emit_unary(compiler, NOT_rm, dst, dstw, src, srcw);

	case SLJIT_NEG:
		if (SLJIT_UNLIKELY(op_flags & SLJIT_KEEP_FLAGS) && !compiler->flags_saved)
			FAIL_IF(emit_save_flags(compiler));
		return emit_unary(compiler, NEG_rm, dst, dstw, src, srcw);

	case SLJIT_CLZ:
		if (SLJIT_UNLIKELY(op_flags & SLJIT_KEEP_FLAGS) && !compiler->flags_saved)
			FAIL_IF(emit_save_flags(compiler));
		return emit_clz(compiler, op_flags, dst, dstw, src, srcw);
	}

	return SLJIT_SUCCESS;

#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
#	undef src_is_ereg
#endif
}

#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)

#define BINARY_IMM(op_imm, op_mr, immw, arg, argw) \
	if (IS_HALFWORD(immw) || compiler->mode32) { \
		inst = emit_x86_instruction(compiler, 1 | EX86_BIN_INS, SLJIT_IMM, immw, arg, argw); \
		FAIL_IF(!inst); \
		*(inst + 1) |= (op_imm); \
	} \
	else { \
		FAIL_IF(emit_load_imm64(compiler, TMP_REG2, immw)); \
		inst = emit_x86_instruction(compiler, 1, TMP_REG2, 0, arg, argw); \
		FAIL_IF(!inst); \
		*inst = (op_mr); \
	}

#define BINARY_EAX_IMM(op_eax_imm, immw) \
	FAIL_IF(emit_do_imm32(compiler, (!compiler->mode32) ? REX_W : 0, (op_eax_imm), immw))

#else

#define BINARY_IMM(op_imm, op_mr, immw, arg, argw) \
	inst = emit_x86_instruction(compiler, 1 | EX86_BIN_INS, SLJIT_IMM, immw, arg, argw); \
	FAIL_IF(!inst); \
	*(inst + 1) |= (op_imm);

#define BINARY_EAX_IMM(op_eax_imm, immw) \
	FAIL_IF(emit_do_imm(compiler, (op_eax_imm), immw))

#endif

static sljit_si emit_cum_binary(struct sljit_compiler *compiler,
	sljit_ub op_rm, sljit_ub op_mr, sljit_ub op_imm, sljit_ub op_eax_imm,
	sljit_si dst, sljit_sw dstw,
	sljit_si src1, sljit_sw src1w,
	sljit_si src2, sljit_sw src2w)
{
	sljit_ub* inst;

	if (dst == SLJIT_UNUSED) {
		EMIT_MOV(compiler, TMP_REG1, 0, src1, src1w);
		if (src2 & SLJIT_IMM) {
			BINARY_IMM(op_imm, op_mr, src2w, TMP_REG1, 0);
		}
		else {
			inst = emit_x86_instruction(compiler, 1, TMP_REG1, 0, src2, src2w);
			FAIL_IF(!inst);
			*inst = op_rm;
		}
		return SLJIT_SUCCESS;
	}

	if (dst == src1 && dstw == src1w) {
		if (src2 & SLJIT_IMM) {
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
			if ((dst == SLJIT_SCRATCH_REG1) && (src2w > 127 || src2w < -128) && (compiler->mode32 || IS_HALFWORD(src2w))) {
#else
			if ((dst == SLJIT_SCRATCH_REG1) && (src2w > 127 || src2w < -128)) {
#endif
				BINARY_EAX_IMM(op_eax_imm, src2w);
			}
			else {
				BINARY_IMM(op_imm, op_mr, src2w, dst, dstw);
			}
		}
		else if (FAST_IS_REG(dst)) {
			inst = emit_x86_instruction(compiler, 1, dst, dstw, src2, src2w);
			FAIL_IF(!inst);
			*inst = op_rm;
		}
		else if (FAST_IS_REG(src2)) {
			/* Special exception for sljit_emit_op_flags. */
			inst = emit_x86_instruction(compiler, 1, src2, src2w, dst, dstw);
			FAIL_IF(!inst);
			*inst = op_mr;
		}
		else {
			EMIT_MOV(compiler, TMP_REG1, 0, src2, src2w);
			inst = emit_x86_instruction(compiler, 1, TMP_REG1, 0, dst, dstw);
			FAIL_IF(!inst);
			*inst = op_mr;
		}
		return SLJIT_SUCCESS;
	}

	/* Only for cumulative operations. */
	if (dst == src2 && dstw == src2w) {
		if (src1 & SLJIT_IMM) {
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
			if ((dst == SLJIT_SCRATCH_REG1) && (src1w > 127 || src1w < -128) && (compiler->mode32 || IS_HALFWORD(src1w))) {
#else
			if ((dst == SLJIT_SCRATCH_REG1) && (src1w > 127 || src1w < -128)) {
#endif
				BINARY_EAX_IMM(op_eax_imm, src1w);
			}
			else {
				BINARY_IMM(op_imm, op_mr, src1w, dst, dstw);
			}
		}
		else if (FAST_IS_REG(dst)) {
			inst = emit_x86_instruction(compiler, 1, dst, dstw, src1, src1w);
			FAIL_IF(!inst);
			*inst = op_rm;
		}
		else if (FAST_IS_REG(src1)) {
			inst = emit_x86_instruction(compiler, 1, src1, src1w, dst, dstw);
			FAIL_IF(!inst);
			*inst = op_mr;
		}
		else {
			EMIT_MOV(compiler, TMP_REG1, 0, src1, src1w);
			inst = emit_x86_instruction(compiler, 1, TMP_REG1, 0, dst, dstw);
			FAIL_IF(!inst);
			*inst = op_mr;
		}
		return SLJIT_SUCCESS;
	}

	/* General version. */
	if (FAST_IS_REG(dst)) {
		EMIT_MOV(compiler, dst, 0, src1, src1w);
		if (src2 & SLJIT_IMM) {
			BINARY_IMM(op_imm, op_mr, src2w, dst, 0);
		}
		else {
			inst = emit_x86_instruction(compiler, 1, dst, 0, src2, src2w);
			FAIL_IF(!inst);
			*inst = op_rm;
		}
	}
	else {
		/* This version requires less memory writing. */
		EMIT_MOV(compiler, TMP_REG1, 0, src1, src1w);
		if (src2 & SLJIT_IMM) {
			BINARY_IMM(op_imm, op_mr, src2w, TMP_REG1, 0);
		}
		else {
			inst = emit_x86_instruction(compiler, 1, TMP_REG1, 0, src2, src2w);
			FAIL_IF(!inst);
			*inst = op_rm;
		}
		EMIT_MOV(compiler, dst, dstw, TMP_REG1, 0);
	}

	return SLJIT_SUCCESS;
}

static sljit_si emit_non_cum_binary(struct sljit_compiler *compiler,
	sljit_ub op_rm, sljit_ub op_mr, sljit_ub op_imm, sljit_ub op_eax_imm,
	sljit_si dst, sljit_sw dstw,
	sljit_si src1, sljit_sw src1w,
	sljit_si src2, sljit_sw src2w)
{
	sljit_ub* inst;

	if (dst == SLJIT_UNUSED) {
		EMIT_MOV(compiler, TMP_REG1, 0, src1, src1w);
		if (src2 & SLJIT_IMM) {
			BINARY_IMM(op_imm, op_mr, src2w, TMP_REG1, 0);
		}
		else {
			inst = emit_x86_instruction(compiler, 1, TMP_REG1, 0, src2, src2w);
			FAIL_IF(!inst);
			*inst = op_rm;
		}
		return SLJIT_SUCCESS;
	}

	if (dst == src1 && dstw == src1w) {
		if (src2 & SLJIT_IMM) {
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
			if ((dst == SLJIT_SCRATCH_REG1) && (src2w > 127 || src2w < -128) && (compiler->mode32 || IS_HALFWORD(src2w))) {
#else
			if ((dst == SLJIT_SCRATCH_REG1) && (src2w > 127 || src2w < -128)) {
#endif
				BINARY_EAX_IMM(op_eax_imm, src2w);
			}
			else {
				BINARY_IMM(op_imm, op_mr, src2w, dst, dstw);
			}
		}
		else if (FAST_IS_REG(dst)) {
			inst = emit_x86_instruction(compiler, 1, dst, dstw, src2, src2w);
			FAIL_IF(!inst);
			*inst = op_rm;
		}
		else if (FAST_IS_REG(src2)) {
			inst = emit_x86_instruction(compiler, 1, src2, src2w, dst, dstw);
			FAIL_IF(!inst);
			*inst = op_mr;
		}
		else {
			EMIT_MOV(compiler, TMP_REG1, 0, src2, src2w);
			inst = emit_x86_instruction(compiler, 1, TMP_REG1, 0, dst, dstw);
			FAIL_IF(!inst);
			*inst = op_mr;
		}
		return SLJIT_SUCCESS;
	}

	/* General version. */
	if (FAST_IS_REG(dst) && dst != src2) {
		EMIT_MOV(compiler, dst, 0, src1, src1w);
		if (src2 & SLJIT_IMM) {
			BINARY_IMM(op_imm, op_mr, src2w, dst, 0);
		}
		else {
			inst = emit_x86_instruction(compiler, 1, dst, 0, src2, src2w);
			FAIL_IF(!inst);
			*inst = op_rm;
		}
	}
	else {
		/* This version requires less memory writing. */
		EMIT_MOV(compiler, TMP_REG1, 0, src1, src1w);
		if (src2 & SLJIT_IMM) {
			BINARY_IMM(op_imm, op_mr, src2w, TMP_REG1, 0);
		}
		else {
			inst = emit_x86_instruction(compiler, 1, TMP_REG1, 0, src2, src2w);
			FAIL_IF(!inst);
			*inst = op_rm;
		}
		EMIT_MOV(compiler, dst, dstw, TMP_REG1, 0);
	}

	return SLJIT_SUCCESS;
}

static sljit_si emit_mul(struct sljit_compiler *compiler,
	sljit_si dst, sljit_sw dstw,
	sljit_si src1, sljit_sw src1w,
	sljit_si src2, sljit_sw src2w)
{
	sljit_ub* inst;
	sljit_si dst_r;

	dst_r = FAST_IS_REG(dst) ? dst : TMP_REG1;

	/* Register destination. */
	if (dst_r == src1 && !(src2 & SLJIT_IMM)) {
		inst = emit_x86_instruction(compiler, 2, dst_r, 0, src2, src2w);
		FAIL_IF(!inst);
		*inst++ = GROUP_0F;
		*inst = IMUL_r_rm;
	}
	else if (dst_r == src2 && !(src1 & SLJIT_IMM)) {
		inst = emit_x86_instruction(compiler, 2, dst_r, 0, src1, src1w);
		FAIL_IF(!inst);
		*inst++ = GROUP_0F;
		*inst = IMUL_r_rm;
	}
	else if (src1 & SLJIT_IMM) {
		if (src2 & SLJIT_IMM) {
			EMIT_MOV(compiler, dst_r, 0, SLJIT_IMM, src2w);
			src2 = dst_r;
			src2w = 0;
		}

		if (src1w <= 127 && src1w >= -128) {
			inst = emit_x86_instruction(compiler, 1, dst_r, 0, src2, src2w);
			FAIL_IF(!inst);
			*inst = IMUL_r_rm_i8;
			inst = (sljit_ub*)ensure_buf(compiler, 1 + 1);
			FAIL_IF(!inst);
			INC_SIZE(1);
			*inst = (sljit_sb)src1w;
		}
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
		else {
			inst = emit_x86_instruction(compiler, 1, dst_r, 0, src2, src2w);
			FAIL_IF(!inst);
			*inst = IMUL_r_rm_i32;
			inst = (sljit_ub*)ensure_buf(compiler, 1 + 4);
			FAIL_IF(!inst);
			INC_SIZE(4);
			*(sljit_sw*)inst = src1w;
		}
#else
		else if (IS_HALFWORD(src1w)) {
			inst = emit_x86_instruction(compiler, 1, dst_r, 0, src2, src2w);
			FAIL_IF(!inst);
			*inst = IMUL_r_rm_i32;
			inst = (sljit_ub*)ensure_buf(compiler, 1 + 4);
			FAIL_IF(!inst);
			INC_SIZE(4);
			*(sljit_si*)inst = (sljit_si)src1w;
		}
		else {
			EMIT_MOV(compiler, TMP_REG2, 0, SLJIT_IMM, src1w);
			if (dst_r != src2)
				EMIT_MOV(compiler, dst_r, 0, src2, src2w);
			inst = emit_x86_instruction(compiler, 2, dst_r, 0, TMP_REG2, 0);
			FAIL_IF(!inst);
			*inst++ = GROUP_0F;
			*inst = IMUL_r_rm;
		}
#endif
	}
	else if (src2 & SLJIT_IMM) {
		/* Note: src1 is NOT immediate. */

		if (src2w <= 127 && src2w >= -128) {
			inst = emit_x86_instruction(compiler, 1, dst_r, 0, src1, src1w);
			FAIL_IF(!inst);
			*inst = IMUL_r_rm_i8;
			inst = (sljit_ub*)ensure_buf(compiler, 1 + 1);
			FAIL_IF(!inst);
			INC_SIZE(1);
			*inst = (sljit_sb)src2w;
		}
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
		else {
			inst = emit_x86_instruction(compiler, 1, dst_r, 0, src1, src1w);
			FAIL_IF(!inst);
			*inst = IMUL_r_rm_i32;
			inst = (sljit_ub*)ensure_buf(compiler, 1 + 4);
			FAIL_IF(!inst);
			INC_SIZE(4);
			*(sljit_sw*)inst = src2w;
		}
#else
		else if (IS_HALFWORD(src2w)) {
			inst = emit_x86_instruction(compiler, 1, dst_r, 0, src1, src1w);
			FAIL_IF(!inst);
			*inst = IMUL_r_rm_i32;
			inst = (sljit_ub*)ensure_buf(compiler, 1 + 4);
			FAIL_IF(!inst);
			INC_SIZE(4);
			*(sljit_si*)inst = (sljit_si)src2w;
		}
		else {
			EMIT_MOV(compiler, TMP_REG2, 0, SLJIT_IMM, src1w);
			if (dst_r != src1)
				EMIT_MOV(compiler, dst_r, 0, src1, src1w);
			inst = emit_x86_instruction(compiler, 2, dst_r, 0, TMP_REG2, 0);
			FAIL_IF(!inst);
			*inst++ = GROUP_0F;
			*inst = IMUL_r_rm;
		}
#endif
	}
	else {
		/* Neither argument is immediate. */
		if (ADDRESSING_DEPENDS_ON(src2, dst_r))
			dst_r = TMP_REG1;
		EMIT_MOV(compiler, dst_r, 0, src1, src1w);
		inst = emit_x86_instruction(compiler, 2, dst_r, 0, src2, src2w);
		FAIL_IF(!inst);
		*inst++ = GROUP_0F;
		*inst = IMUL_r_rm;
	}

	if (dst_r == TMP_REG1)
		EMIT_MOV(compiler, dst, dstw, TMP_REG1, 0);

	return SLJIT_SUCCESS;
}

static sljit_si emit_lea_binary(struct sljit_compiler *compiler, sljit_si keep_flags,
	sljit_si dst, sljit_sw dstw,
	sljit_si src1, sljit_sw src1w,
	sljit_si src2, sljit_sw src2w)
{
	sljit_ub* inst;
	sljit_si dst_r, done = 0;

	/* These cases better be left to handled by normal way. */
	if (!keep_flags) {
		if (dst == src1 && dstw == src1w)
			return SLJIT_ERR_UNSUPPORTED;
		if (dst == src2 && dstw == src2w)
			return SLJIT_ERR_UNSUPPORTED;
	}

	dst_r = FAST_IS_REG(dst) ? dst : TMP_REG1;

	if (FAST_IS_REG(src1)) {
		if (FAST_IS_REG(src2)) {
			inst = emit_x86_instruction(compiler, 1, dst_r, 0, SLJIT_MEM2(src1, src2), 0);
			FAIL_IF(!inst);
			*inst = LEA_r_m;
			done = 1;
		}
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
		if ((src2 & SLJIT_IMM) && (compiler->mode32 || IS_HALFWORD(src2w))) {
			inst = emit_x86_instruction(compiler, 1, dst_r, 0, SLJIT_MEM1(src1), (sljit_si)src2w);
#else
		if (src2 & SLJIT_IMM) {
			inst = emit_x86_instruction(compiler, 1, dst_r, 0, SLJIT_MEM1(src1), src2w);
#endif
			FAIL_IF(!inst);
			*inst = LEA_r_m;
			done = 1;
		}
	}
	else if (FAST_IS_REG(src2)) {
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
		if ((src1 & SLJIT_IMM) && (compiler->mode32 || IS_HALFWORD(src1w))) {
			inst = emit_x86_instruction(compiler, 1, dst_r, 0, SLJIT_MEM1(src2), (sljit_si)src1w);
#else
		if (src1 & SLJIT_IMM) {
			inst = emit_x86_instruction(compiler, 1, dst_r, 0, SLJIT_MEM1(src2), src1w);
#endif
			FAIL_IF(!inst);
			*inst = LEA_r_m;
			done = 1;
		}
	}

	if (done) {
		if (dst_r == TMP_REG1)
			return emit_mov(compiler, dst, dstw, TMP_REG1, 0);
		return SLJIT_SUCCESS;
	}
	return SLJIT_ERR_UNSUPPORTED;
}

static sljit_si emit_cmp_binary(struct sljit_compiler *compiler,
	sljit_si src1, sljit_sw src1w,
	sljit_si src2, sljit_sw src2w)
{
	sljit_ub* inst;

#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
	if (src1 == SLJIT_SCRATCH_REG1 && (src2 & SLJIT_IMM) && (src2w > 127 || src2w < -128) && (compiler->mode32 || IS_HALFWORD(src2w))) {
#else
	if (src1 == SLJIT_SCRATCH_REG1 && (src2 & SLJIT_IMM) && (src2w > 127 || src2w < -128)) {
#endif
		BINARY_EAX_IMM(CMP_EAX_i32, src2w);
		return SLJIT_SUCCESS;
	}

	if (FAST_IS_REG(src1)) {
		if (src2 & SLJIT_IMM) {
			BINARY_IMM(CMP, CMP_rm_r, src2w, src1, 0);
		}
		else {
			inst = emit_x86_instruction(compiler, 1, src1, 0, src2, src2w);
			FAIL_IF(!inst);
			*inst = CMP_r_rm;
		}
		return SLJIT_SUCCESS;
	}

	if (FAST_IS_REG(src2) && !(src1 & SLJIT_IMM)) {
		inst = emit_x86_instruction(compiler, 1, src2, 0, src1, src1w);
		FAIL_IF(!inst);
		*inst = CMP_rm_r;
		return SLJIT_SUCCESS;
	}

	if (src2 & SLJIT_IMM) {
		if (src1 & SLJIT_IMM) {
			EMIT_MOV(compiler, TMP_REG1, 0, src1, src1w);
			src1 = TMP_REG1;
			src1w = 0;
		}
		BINARY_IMM(CMP, CMP_rm_r, src2w, src1, src1w);
	}
	else {
		EMIT_MOV(compiler, TMP_REG1, 0, src1, src1w);
		inst = emit_x86_instruction(compiler, 1, TMP_REG1, 0, src2, src2w);
		FAIL_IF(!inst);
		*inst = CMP_r_rm;
	}
	return SLJIT_SUCCESS;
}

static sljit_si emit_test_binary(struct sljit_compiler *compiler,
	sljit_si src1, sljit_sw src1w,
	sljit_si src2, sljit_sw src2w)
{
	sljit_ub* inst;

#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
	if (src1 == SLJIT_SCRATCH_REG1 && (src2 & SLJIT_IMM) && (src2w > 127 || src2w < -128) && (compiler->mode32 || IS_HALFWORD(src2w))) {
#else
	if (src1 == SLJIT_SCRATCH_REG1 && (src2 & SLJIT_IMM) && (src2w > 127 || src2w < -128)) {
#endif
		BINARY_EAX_IMM(TEST_EAX_i32, src2w);
		return SLJIT_SUCCESS;
	}

#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
	if (src2 == SLJIT_SCRATCH_REG1 && (src2 & SLJIT_IMM) && (src1w > 127 || src1w < -128) && (compiler->mode32 || IS_HALFWORD(src1w))) {
#else
	if (src2 == SLJIT_SCRATCH_REG1 && (src1 & SLJIT_IMM) && (src1w > 127 || src1w < -128)) {
#endif
		BINARY_EAX_IMM(TEST_EAX_i32, src1w);
		return SLJIT_SUCCESS;
	}

	if (FAST_IS_REG(src1)) {
		if (src2 & SLJIT_IMM) {
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
			if (IS_HALFWORD(src2w) || compiler->mode32) {
				inst = emit_x86_instruction(compiler, 1, SLJIT_IMM, src2w, src1, 0);
				FAIL_IF(!inst);
				*inst = GROUP_F7;
			}
			else {
				FAIL_IF(emit_load_imm64(compiler, TMP_REG2, src2w));
				inst = emit_x86_instruction(compiler, 1, TMP_REG2, 0, src1, 0);
				FAIL_IF(!inst);
				*inst = TEST_rm_r;
			}
#else
			inst = emit_x86_instruction(compiler, 1, SLJIT_IMM, src2w, src1, 0);
			FAIL_IF(!inst);
			*inst = GROUP_F7;
#endif
		}
		else {
			inst = emit_x86_instruction(compiler, 1, src1, 0, src2, src2w);
			FAIL_IF(!inst);
			*inst = TEST_rm_r;
		}
		return SLJIT_SUCCESS;
	}

	if (FAST_IS_REG(src2)) {
		if (src1 & SLJIT_IMM) {
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
			if (IS_HALFWORD(src1w) || compiler->mode32) {
				inst = emit_x86_instruction(compiler, 1, SLJIT_IMM, src1w, src2, 0);
				FAIL_IF(!inst);
				*inst = GROUP_F7;
			}
			else {
				FAIL_IF(emit_load_imm64(compiler, TMP_REG2, src1w));
				inst = emit_x86_instruction(compiler, 1, TMP_REG2, 0, src2, 0);
				FAIL_IF(!inst);
				*inst = TEST_rm_r;
			}
#else
			inst = emit_x86_instruction(compiler, 1, src1, src1w, src2, 0);
			FAIL_IF(!inst);
			*inst = GROUP_F7;
#endif
		}
		else {
			inst = emit_x86_instruction(compiler, 1, src2, 0, src1, src1w);
			FAIL_IF(!inst);
			*inst = TEST_rm_r;
		}
		return SLJIT_SUCCESS;
	}

	EMIT_MOV(compiler, TMP_REG1, 0, src1, src1w);
	if (src2 & SLJIT_IMM) {
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
		if (IS_HALFWORD(src2w) || compiler->mode32) {
			inst = emit_x86_instruction(compiler, 1, SLJIT_IMM, src2w, TMP_REG1, 0);
			FAIL_IF(!inst);
			*inst = GROUP_F7;
		}
		else {
			FAIL_IF(emit_load_imm64(compiler, TMP_REG2, src2w));
			inst = emit_x86_instruction(compiler, 1, TMP_REG2, 0, TMP_REG1, 0);
			FAIL_IF(!inst);
			*inst = TEST_rm_r;
		}
#else
		inst = emit_x86_instruction(compiler, 1, SLJIT_IMM, src2w, TMP_REG1, 0);
		FAIL_IF(!inst);
		*inst = GROUP_F7;
#endif
	}
	else {
		inst = emit_x86_instruction(compiler, 1, TMP_REG1, 0, src2, src2w);
		FAIL_IF(!inst);
		*inst = TEST_rm_r;
	}
	return SLJIT_SUCCESS;
}

static sljit_si emit_shift(struct sljit_compiler *compiler,
	sljit_ub mode,
	sljit_si dst, sljit_sw dstw,
	sljit_si src1, sljit_sw src1w,
	sljit_si src2, sljit_sw src2w)
{
	sljit_ub* inst;

	if ((src2 & SLJIT_IMM) || (src2 == SLJIT_PREF_SHIFT_REG)) {
		if (dst == src1 && dstw == src1w) {
			inst = emit_x86_instruction(compiler, 1 | EX86_SHIFT_INS, src2, src2w, dst, dstw);
			FAIL_IF(!inst);
			*inst |= mode;
			return SLJIT_SUCCESS;
		}
		if (dst == SLJIT_UNUSED) {
			EMIT_MOV(compiler, TMP_REG1, 0, src1, src1w);
			inst = emit_x86_instruction(compiler, 1 | EX86_SHIFT_INS, src2, src2w, TMP_REG1, 0);
			FAIL_IF(!inst);
			*inst |= mode;
			return SLJIT_SUCCESS;
		}
		if (dst == SLJIT_PREF_SHIFT_REG && src2 == SLJIT_PREF_SHIFT_REG) {
			EMIT_MOV(compiler, TMP_REG1, 0, src1, src1w);
			inst = emit_x86_instruction(compiler, 1 | EX86_SHIFT_INS, SLJIT_PREF_SHIFT_REG, 0, TMP_REG1, 0);
			FAIL_IF(!inst);
			*inst |= mode;
			EMIT_MOV(compiler, SLJIT_PREF_SHIFT_REG, 0, TMP_REG1, 0);
			return SLJIT_SUCCESS;
		}
		if (FAST_IS_REG(dst)) {
			EMIT_MOV(compiler, dst, 0, src1, src1w);
			inst = emit_x86_instruction(compiler, 1 | EX86_SHIFT_INS, src2, src2w, dst, 0);
			FAIL_IF(!inst);
			*inst |= mode;
			return SLJIT_SUCCESS;
		}

		EMIT_MOV(compiler, TMP_REG1, 0, src1, src1w);
		inst = emit_x86_instruction(compiler, 1 | EX86_SHIFT_INS, src2, src2w, TMP_REG1, 0);
		FAIL_IF(!inst);
		*inst |= mode;
		EMIT_MOV(compiler, dst, dstw, TMP_REG1, 0);
		return SLJIT_SUCCESS;
	}

	if (dst == SLJIT_PREF_SHIFT_REG) {
		EMIT_MOV(compiler, TMP_REG1, 0, src1, src1w);
		EMIT_MOV(compiler, SLJIT_PREF_SHIFT_REG, 0, src2, src2w);
		inst = emit_x86_instruction(compiler, 1 | EX86_SHIFT_INS, SLJIT_PREF_SHIFT_REG, 0, TMP_REG1, 0);
		FAIL_IF(!inst);
		*inst |= mode;
		EMIT_MOV(compiler, SLJIT_PREF_SHIFT_REG, 0, TMP_REG1, 0);
	}
	else if (FAST_IS_REG(dst) && dst != src2 && !ADDRESSING_DEPENDS_ON(src2, dst)) {
		if (src1 != dst)
			EMIT_MOV(compiler, dst, 0, src1, src1w);
		EMIT_MOV(compiler, TMP_REG1, 0, SLJIT_PREF_SHIFT_REG, 0);
		EMIT_MOV(compiler, SLJIT_PREF_SHIFT_REG, 0, src2, src2w);
		inst = emit_x86_instruction(compiler, 1 | EX86_SHIFT_INS, SLJIT_PREF_SHIFT_REG, 0, dst, 0);
		FAIL_IF(!inst);
		*inst |= mode;
		EMIT_MOV(compiler, SLJIT_PREF_SHIFT_REG, 0, TMP_REG1, 0);
	}
	else {
		/* This case is really difficult, since ecx itself may used for
		   addressing, and we must ensure to work even in that case. */
		EMIT_MOV(compiler, TMP_REG1, 0, src1, src1w);
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
		EMIT_MOV(compiler, TMP_REG2, 0, SLJIT_PREF_SHIFT_REG, 0);
#else
		/* [esp+0] contains the flags. */
		EMIT_MOV(compiler, SLJIT_MEM1(SLJIT_LOCALS_REG), sizeof(sljit_sw), SLJIT_PREF_SHIFT_REG, 0);
#endif
		EMIT_MOV(compiler, SLJIT_PREF_SHIFT_REG, 0, src2, src2w);
		inst = emit_x86_instruction(compiler, 1 | EX86_SHIFT_INS, SLJIT_PREF_SHIFT_REG, 0, TMP_REG1, 0);
		FAIL_IF(!inst);
		*inst |= mode;
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
		EMIT_MOV(compiler, SLJIT_PREF_SHIFT_REG, 0, TMP_REG2, 0);
#else
		EMIT_MOV(compiler, SLJIT_PREF_SHIFT_REG, 0, SLJIT_MEM1(SLJIT_LOCALS_REG), sizeof(sljit_sw));
#endif
		EMIT_MOV(compiler, dst, dstw, TMP_REG1, 0);
	}

	return SLJIT_SUCCESS;
}

static sljit_si emit_shift_with_flags(struct sljit_compiler *compiler,
	sljit_ub mode, sljit_si set_flags,
	sljit_si dst, sljit_sw dstw,
	sljit_si src1, sljit_sw src1w,
	sljit_si src2, sljit_sw src2w)
{
	/* The CPU does not set flags if the shift count is 0. */
	if (src2 & SLJIT_IMM) {
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
		if ((src2w & 0x3f) != 0 || (compiler->mode32 && (src2w & 0x1f) != 0))
			return emit_shift(compiler, mode, dst, dstw, src1, src1w, src2, src2w);
#else
		if ((src2w & 0x1f) != 0)
			return emit_shift(compiler, mode, dst, dstw, src1, src1w, src2, src2w);
#endif
		if (!set_flags)
			return emit_mov(compiler, dst, dstw, src1, src1w);
		/* OR dst, src, 0 */
		return emit_cum_binary(compiler, OR_r_rm, OR_rm_r, OR, OR_EAX_i32,
			dst, dstw, src1, src1w, SLJIT_IMM, 0);
	}

	if (!set_flags)
		return emit_shift(compiler, mode, dst, dstw, src1, src1w, src2, src2w);

	if (!FAST_IS_REG(dst))
		FAIL_IF(emit_cmp_binary(compiler, src1, src1w, SLJIT_IMM, 0));

	FAIL_IF(emit_shift(compiler,mode, dst, dstw, src1, src1w, src2, src2w));

	if (FAST_IS_REG(dst))
		return emit_cmp_binary(compiler, dst, dstw, SLJIT_IMM, 0);
	return SLJIT_SUCCESS;
}

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_emit_op2(struct sljit_compiler *compiler, sljit_si op,
	sljit_si dst, sljit_sw dstw,
	sljit_si src1, sljit_sw src1w,
	sljit_si src2, sljit_sw src2w)
{
	CHECK_ERROR();
	check_sljit_emit_op2(compiler, op, dst, dstw, src1, src1w, src2, src2w);
	ADJUST_LOCAL_OFFSET(dst, dstw);
	ADJUST_LOCAL_OFFSET(src1, src1w);
	ADJUST_LOCAL_OFFSET(src2, src2w);

	CHECK_EXTRA_REGS(dst, dstw, (void)0);
	CHECK_EXTRA_REGS(src1, src1w, (void)0);
	CHECK_EXTRA_REGS(src2, src2w, (void)0);
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
	compiler->mode32 = op & SLJIT_INT_OP;
#endif

	if (GET_OPCODE(op) >= SLJIT_MUL) {
		if (SLJIT_UNLIKELY(GET_FLAGS(op)))
			compiler->flags_saved = 0;
		else if (SLJIT_UNLIKELY(op & SLJIT_KEEP_FLAGS) && !compiler->flags_saved)
			FAIL_IF(emit_save_flags(compiler));
	}

	switch (GET_OPCODE(op)) {
	case SLJIT_ADD:
		if (!GET_FLAGS(op)) {
			if (emit_lea_binary(compiler, op & SLJIT_KEEP_FLAGS, dst, dstw, src1, src1w, src2, src2w) != SLJIT_ERR_UNSUPPORTED)
				return compiler->error;
		}
		else
			compiler->flags_saved = 0;
		if (SLJIT_UNLIKELY(op & SLJIT_KEEP_FLAGS) && !compiler->flags_saved)
			FAIL_IF(emit_save_flags(compiler));
		return emit_cum_binary(compiler, ADD_r_rm, ADD_rm_r, ADD, ADD_EAX_i32,
			dst, dstw, src1, src1w, src2, src2w);
	case SLJIT_ADDC:
		if (SLJIT_UNLIKELY(compiler->flags_saved)) /* C flag must be restored. */
			FAIL_IF(emit_restore_flags(compiler, 1));
		else if (SLJIT_UNLIKELY(op & SLJIT_KEEP_FLAGS))
			FAIL_IF(emit_save_flags(compiler));
		if (SLJIT_UNLIKELY(GET_FLAGS(op)))
			compiler->flags_saved = 0;
		return emit_cum_binary(compiler, ADC_r_rm, ADC_rm_r, ADC, ADC_EAX_i32,
			dst, dstw, src1, src1w, src2, src2w);
	case SLJIT_SUB:
		if (!GET_FLAGS(op)) {
			if ((src2 & SLJIT_IMM) && emit_lea_binary(compiler, op & SLJIT_KEEP_FLAGS, dst, dstw, src1, src1w, SLJIT_IMM, -src2w) != SLJIT_ERR_UNSUPPORTED)
				return compiler->error;
		}
		else
			compiler->flags_saved = 0;
		if (SLJIT_UNLIKELY(op & SLJIT_KEEP_FLAGS) && !compiler->flags_saved)
			FAIL_IF(emit_save_flags(compiler));
		if (dst == SLJIT_UNUSED)
			return emit_cmp_binary(compiler, src1, src1w, src2, src2w);
		return emit_non_cum_binary(compiler, SUB_r_rm, SUB_rm_r, SUB, SUB_EAX_i32,
			dst, dstw, src1, src1w, src2, src2w);
	case SLJIT_SUBC:
		if (SLJIT_UNLIKELY(compiler->flags_saved)) /* C flag must be restored. */
			FAIL_IF(emit_restore_flags(compiler, 1));
		else if (SLJIT_UNLIKELY(op & SLJIT_KEEP_FLAGS))
			FAIL_IF(emit_save_flags(compiler));
		if (SLJIT_UNLIKELY(GET_FLAGS(op)))
			compiler->flags_saved = 0;
		return emit_non_cum_binary(compiler, SBB_r_rm, SBB_rm_r, SBB, SBB_EAX_i32,
			dst, dstw, src1, src1w, src2, src2w);
	case SLJIT_MUL:
		return emit_mul(compiler, dst, dstw, src1, src1w, src2, src2w);
	case SLJIT_AND:
		if (dst == SLJIT_UNUSED)
			return emit_test_binary(compiler, src1, src1w, src2, src2w);
		return emit_cum_binary(compiler, AND_r_rm, AND_rm_r, AND, AND_EAX_i32,
			dst, dstw, src1, src1w, src2, src2w);
	case SLJIT_OR:
		return emit_cum_binary(compiler, OR_r_rm, OR_rm_r, OR, OR_EAX_i32,
			dst, dstw, src1, src1w, src2, src2w);
	case SLJIT_XOR:
		return emit_cum_binary(compiler, XOR_r_rm, XOR_rm_r, XOR, XOR_EAX_i32,
			dst, dstw, src1, src1w, src2, src2w);
	case SLJIT_SHL:
		return emit_shift_with_flags(compiler, SHL, GET_FLAGS(op),
			dst, dstw, src1, src1w, src2, src2w);
	case SLJIT_LSHR:
		return emit_shift_with_flags(compiler, SHR, GET_FLAGS(op),
			dst, dstw, src1, src1w, src2, src2w);
	case SLJIT_ASHR:
		return emit_shift_with_flags(compiler, SAR, GET_FLAGS(op),
			dst, dstw, src1, src1w, src2, src2w);
	}

	return SLJIT_SUCCESS;
}

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_get_register_index(sljit_si reg)
{
	check_sljit_get_register_index(reg);
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
	if (reg == SLJIT_TEMPORARY_EREG1 || reg == SLJIT_TEMPORARY_EREG2
			|| reg == SLJIT_SAVED_EREG1 || reg == SLJIT_SAVED_EREG2)
		return -1;
#endif
	return reg_map[reg];
}

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_get_float_register_index(sljit_si reg)
{
	check_sljit_get_float_register_index(reg);
	return reg;
}

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_emit_op_custom(struct sljit_compiler *compiler,
	void *instruction, sljit_si size)
{
	sljit_ub *inst;

	CHECK_ERROR();
	check_sljit_emit_op_custom(compiler, instruction, size);
	SLJIT_ASSERT(size > 0 && size < 16);

	inst = (sljit_ub*)ensure_buf(compiler, 1 + size);
	FAIL_IF(!inst);
	INC_SIZE(size);
	SLJIT_MEMMOVE(inst, instruction, size);
	return SLJIT_SUCCESS;
}

/* --------------------------------------------------------------------- */
/*  Floating point operators                                             */
/* --------------------------------------------------------------------- */

#if (defined SLJIT_SSE2 && SLJIT_SSE2)

/* Alignment + 2 * 16 bytes. */
static sljit_si sse2_data[3 + (4 + 4) * 2];
static sljit_si *sse2_buffer;

static void init_compiler(void)
{
	sse2_buffer = (sljit_si*)(((sljit_uw)sse2_data + 15) & ~0xf);
	/* Single precision constants. */
	sse2_buffer[0] = 0x80000000;
	sse2_buffer[4] = 0x7fffffff;
	/* Double precision constants. */
	sse2_buffer[8] = 0;
	sse2_buffer[9] = 0x80000000;
	sse2_buffer[12] = 0xffffffff;
	sse2_buffer[13] = 0x7fffffff;
}

#endif

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_is_fpu_available(void)
{
#ifdef SLJIT_IS_FPU_AVAILABLE
	return SLJIT_IS_FPU_AVAILABLE;
#elif (defined SLJIT_SSE2 && SLJIT_SSE2)
#if (defined SLJIT_DETECT_SSE2 && SLJIT_DETECT_SSE2)
	if (cpu_has_sse2 == -1)
		get_cpu_features();
	return cpu_has_sse2;
#else /* SLJIT_DETECT_SSE2 */
	return 1;
#endif /* SLJIT_DETECT_SSE2 */
#else /* SLJIT_SSE2 */
	return 0;
#endif
}

#if (defined SLJIT_SSE2 && SLJIT_SSE2)

static sljit_si emit_sse2(struct sljit_compiler *compiler, sljit_ub opcode,
	sljit_si single, sljit_si xmm1, sljit_si xmm2, sljit_sw xmm2w)
{
	sljit_ub *inst;

	inst = emit_x86_instruction(compiler, 2 | (single ? EX86_PREF_F3 : EX86_PREF_F2) | EX86_SSE2, xmm1, 0, xmm2, xmm2w);
	FAIL_IF(!inst);
	*inst++ = GROUP_0F;
	*inst = opcode;
	return SLJIT_SUCCESS;
}

static sljit_si emit_sse2_logic(struct sljit_compiler *compiler, sljit_ub opcode,
	sljit_si pref66, sljit_si xmm1, sljit_si xmm2, sljit_sw xmm2w)
{
	sljit_ub *inst;

	inst = emit_x86_instruction(compiler, 2 | (pref66 ? EX86_PREF_66 : 0) | EX86_SSE2, xmm1, 0, xmm2, xmm2w);
	FAIL_IF(!inst);
	*inst++ = GROUP_0F;
	*inst = opcode;
	return SLJIT_SUCCESS;
}

static SLJIT_INLINE sljit_si emit_sse2_load(struct sljit_compiler *compiler,
	sljit_si single, sljit_si dst, sljit_si src, sljit_sw srcw)
{
	return emit_sse2(compiler, MOVSD_x_xm, single, dst, src, srcw);
}

static SLJIT_INLINE sljit_si emit_sse2_store(struct sljit_compiler *compiler,
	sljit_si single, sljit_si dst, sljit_sw dstw, sljit_si src)
{
	return emit_sse2(compiler, MOVSD_xm_x, single, src, dst, dstw);
}

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_emit_fop1(struct sljit_compiler *compiler, sljit_si op,
	sljit_si dst, sljit_sw dstw,
	sljit_si src, sljit_sw srcw)
{
	sljit_si dst_r;

	CHECK_ERROR();
	check_sljit_emit_fop1(compiler, op, dst, dstw, src, srcw);

#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
	compiler->mode32 = 1;
#endif

	if (GET_OPCODE(op) == SLJIT_CMPD) {
		compiler->flags_saved = 0;
		if (FAST_IS_REG(dst))
			dst_r = dst;
		else {
			dst_r = TMP_FREG;
			FAIL_IF(emit_sse2_load(compiler, op & SLJIT_SINGLE_OP, dst_r, dst, dstw));
		}
		return emit_sse2_logic(compiler, UCOMISD_x_xm, !(op & SLJIT_SINGLE_OP), dst_r, src, srcw);
	}

	if (op == SLJIT_MOVD) {
		if (FAST_IS_REG(dst))
			return emit_sse2_load(compiler, op & SLJIT_SINGLE_OP, dst, src, srcw);
		if (FAST_IS_REG(src))
			return emit_sse2_store(compiler, op & SLJIT_SINGLE_OP, dst, dstw, src);
		FAIL_IF(emit_sse2_load(compiler, op & SLJIT_SINGLE_OP, TMP_FREG, src, srcw));
		return emit_sse2_store(compiler, op & SLJIT_SINGLE_OP, dst, dstw, TMP_FREG);
	}

	if (SLOW_IS_REG(dst)) {
		dst_r = dst;
		if (dst != src)
			FAIL_IF(emit_sse2_load(compiler, op & SLJIT_SINGLE_OP, dst_r, src, srcw));
	}
	else {
		dst_r = TMP_FREG;
		FAIL_IF(emit_sse2_load(compiler, op & SLJIT_SINGLE_OP, dst_r, src, srcw));
	}

	switch (GET_OPCODE(op)) {
	case SLJIT_NEGD:
		FAIL_IF(emit_sse2_logic(compiler, XORPD_x_xm, 1, dst_r, SLJIT_MEM0(), (sljit_sw)(op & SLJIT_SINGLE_OP ? sse2_buffer : sse2_buffer + 8)));
		break;

	case SLJIT_ABSD:
		FAIL_IF(emit_sse2_logic(compiler, ANDPD_x_xm, 1, dst_r, SLJIT_MEM0(), (sljit_sw)(op & SLJIT_SINGLE_OP ? sse2_buffer + 4 : sse2_buffer + 12)));
		break;
	}

	if (dst_r == TMP_FREG)
		return emit_sse2_store(compiler, op & SLJIT_SINGLE_OP, dst, dstw, TMP_FREG);
	return SLJIT_SUCCESS;
}

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_emit_fop2(struct sljit_compiler *compiler, sljit_si op,
	sljit_si dst, sljit_sw dstw,
	sljit_si src1, sljit_sw src1w,
	sljit_si src2, sljit_sw src2w)
{
	sljit_si dst_r;

	CHECK_ERROR();
	check_sljit_emit_fop2(compiler, op, dst, dstw, src1, src1w, src2, src2w);

#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
	compiler->mode32 = 1;
#endif

	if (FAST_IS_REG(dst)) {
		dst_r = dst;
		if (dst == src1)
			; /* Do nothing here. */
		else if (dst == src2 && (op == SLJIT_ADDD || op == SLJIT_MULD)) {
			/* Swap arguments. */
			src2 = src1;
			src2w = src1w;
		}
		else if (dst != src2)
			FAIL_IF(emit_sse2_load(compiler, op & SLJIT_SINGLE_OP, dst_r, src1, src1w));
		else {
			dst_r = TMP_FREG;
			FAIL_IF(emit_sse2_load(compiler, op & SLJIT_SINGLE_OP, TMP_FREG, src1, src1w));
		}
	}
	else {
		dst_r = TMP_FREG;
		FAIL_IF(emit_sse2_load(compiler, op & SLJIT_SINGLE_OP, TMP_FREG, src1, src1w));
	}

	switch (GET_OPCODE(op)) {
	case SLJIT_ADDD:
		FAIL_IF(emit_sse2(compiler, ADDSD_x_xm, op & SLJIT_SINGLE_OP, dst_r, src2, src2w));
		break;

	case SLJIT_SUBD:
		FAIL_IF(emit_sse2(compiler, SUBSD_x_xm, op & SLJIT_SINGLE_OP, dst_r, src2, src2w));
		break;

	case SLJIT_MULD:
		FAIL_IF(emit_sse2(compiler, MULSD_x_xm, op & SLJIT_SINGLE_OP, dst_r, src2, src2w));
		break;

	case SLJIT_DIVD:
		FAIL_IF(emit_sse2(compiler, DIVSD_x_xm, op & SLJIT_SINGLE_OP, dst_r, src2, src2w));
		break;
	}

	if (dst_r == TMP_FREG)
		return emit_sse2_store(compiler, op & SLJIT_SINGLE_OP, dst, dstw, TMP_FREG);
	return SLJIT_SUCCESS;
}

#else

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_emit_fop1(struct sljit_compiler *compiler, sljit_si op,
	sljit_si dst, sljit_sw dstw,
	sljit_si src, sljit_sw srcw)
{
	CHECK_ERROR();
	/* Should cause an assertion fail. */
	check_sljit_emit_fop1(compiler, op, dst, dstw, src, srcw);
	compiler->error = SLJIT_ERR_UNSUPPORTED;
	return SLJIT_ERR_UNSUPPORTED;
}

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_emit_fop2(struct sljit_compiler *compiler, sljit_si op,
	sljit_si dst, sljit_sw dstw,
	sljit_si src1, sljit_sw src1w,
	sljit_si src2, sljit_sw src2w)
{
	CHECK_ERROR();
	/* Should cause an assertion fail. */
	check_sljit_emit_fop2(compiler, op, dst, dstw, src1, src1w, src2, src2w);
	compiler->error = SLJIT_ERR_UNSUPPORTED;
	return SLJIT_ERR_UNSUPPORTED;
}

#endif

/* --------------------------------------------------------------------- */
/*  Conditional instructions                                             */
/* --------------------------------------------------------------------- */

SLJIT_API_FUNC_ATTRIBUTE struct sljit_label* sljit_emit_label(struct sljit_compiler *compiler)
{
	sljit_ub *inst;
	struct sljit_label *label;

	CHECK_ERROR_PTR();
	check_sljit_emit_label(compiler);

	/* We should restore the flags before the label,
	   since other taken jumps has their own flags as well. */
	if (SLJIT_UNLIKELY(compiler->flags_saved))
		PTR_FAIL_IF(emit_restore_flags(compiler, 0));

	if (compiler->last_label && compiler->last_label->size == compiler->size)
		return compiler->last_label;

	label = (struct sljit_label*)ensure_abuf(compiler, sizeof(struct sljit_label));
	PTR_FAIL_IF(!label);
	set_label(label, compiler);

	inst = (sljit_ub*)ensure_buf(compiler, 2);
	PTR_FAIL_IF(!inst);

	*inst++ = 0;
	*inst++ = 0;

	return label;
}

SLJIT_API_FUNC_ATTRIBUTE struct sljit_jump* sljit_emit_jump(struct sljit_compiler *compiler, sljit_si type)
{
	sljit_ub *inst;
	struct sljit_jump *jump;

	CHECK_ERROR_PTR();
	check_sljit_emit_jump(compiler, type);

	if (SLJIT_UNLIKELY(compiler->flags_saved)) {
		if ((type & 0xff) <= SLJIT_JUMP)
			PTR_FAIL_IF(emit_restore_flags(compiler, 0));
		compiler->flags_saved = 0;
	}

	jump = (struct sljit_jump*)ensure_abuf(compiler, sizeof(struct sljit_jump));
	PTR_FAIL_IF_NULL(jump);
	set_jump(jump, compiler, type & SLJIT_REWRITABLE_JUMP);
	type &= 0xff;

	if (type >= SLJIT_CALL1)
		PTR_FAIL_IF(call_with_args(compiler, type));

	/* Worst case size. */
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
	compiler->size += (type >= SLJIT_JUMP) ? 5 : 6;
#else
	compiler->size += (type >= SLJIT_JUMP) ? (10 + 3) : (2 + 10 + 3);
#endif

	inst = (sljit_ub*)ensure_buf(compiler, 2);
	PTR_FAIL_IF_NULL(inst);

	*inst++ = 0;
	*inst++ = type + 4;
	return jump;
}

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_emit_ijump(struct sljit_compiler *compiler, sljit_si type, sljit_si src, sljit_sw srcw)
{
	sljit_ub *inst;
	struct sljit_jump *jump;

	CHECK_ERROR();
	check_sljit_emit_ijump(compiler, type, src, srcw);
	ADJUST_LOCAL_OFFSET(src, srcw);

	CHECK_EXTRA_REGS(src, srcw, (void)0);

	if (SLJIT_UNLIKELY(compiler->flags_saved)) {
		if (type <= SLJIT_JUMP)
			FAIL_IF(emit_restore_flags(compiler, 0));
		compiler->flags_saved = 0;
	}

	if (type >= SLJIT_CALL1) {
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
#if (defined SLJIT_X86_32_FASTCALL && SLJIT_X86_32_FASTCALL)
		if (src == SLJIT_SCRATCH_REG3) {
			EMIT_MOV(compiler, TMP_REG1, 0, src, 0);
			src = TMP_REG1;
		}
		if (src == SLJIT_MEM1(SLJIT_LOCALS_REG) && type >= SLJIT_CALL3)
			srcw += sizeof(sljit_sw);
#endif
#endif
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64) && defined(_WIN64)
		if (src == SLJIT_SCRATCH_REG3) {
			EMIT_MOV(compiler, TMP_REG1, 0, src, 0);
			src = TMP_REG1;
		}
#endif
		FAIL_IF(call_with_args(compiler, type));
	}

	if (src == SLJIT_IMM) {
		jump = (struct sljit_jump*)ensure_abuf(compiler, sizeof(struct sljit_jump));
		FAIL_IF_NULL(jump);
		set_jump(jump, compiler, JUMP_ADDR);
		jump->u.target = srcw;

		/* Worst case size. */
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
		compiler->size += 5;
#else
		compiler->size += 10 + 3;
#endif

		inst = (sljit_ub*)ensure_buf(compiler, 2);
		FAIL_IF_NULL(inst);

		*inst++ = 0;
		*inst++ = type + 4;
	}
	else {
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
		/* REX_W is not necessary (src is not immediate). */
		compiler->mode32 = 1;
#endif
		inst = emit_x86_instruction(compiler, 1, 0, 0, src, srcw);
		FAIL_IF(!inst);
		*inst++ = GROUP_FF;
		*inst |= (type >= SLJIT_FAST_CALL) ? CALL_rm : JMP_rm;
	}
	return SLJIT_SUCCESS;
}

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_emit_op_flags(struct sljit_compiler *compiler, sljit_si op,
	sljit_si dst, sljit_sw dstw,
	sljit_si src, sljit_sw srcw,
	sljit_si type)
{
	sljit_ub *inst;
	sljit_ub cond_set = 0;
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
	sljit_si reg;
#else
	/* CHECK_EXTRA_REGS migh overwrite these values. */
	sljit_si dst_save = dst;
	sljit_sw dstw_save = dstw;
#endif

	CHECK_ERROR();
	check_sljit_emit_op_flags(compiler, op, dst, dstw, src, srcw, type);

	if (dst == SLJIT_UNUSED)
		return SLJIT_SUCCESS;

	ADJUST_LOCAL_OFFSET(dst, dstw);
	CHECK_EXTRA_REGS(dst, dstw, (void)0);
	if (SLJIT_UNLIKELY(compiler->flags_saved))
		FAIL_IF(emit_restore_flags(compiler, op & SLJIT_KEEP_FLAGS));

	/* setcc = jcc + 0x10. */
	cond_set = get_jump_code(type) + 0x10;

#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
	if (GET_OPCODE(op) == SLJIT_OR && !GET_ALL_FLAGS(op) && FAST_IS_REG(dst) && dst == src) {
		inst = (sljit_ub*)ensure_buf(compiler, 1 + 4 + 3);
		FAIL_IF(!inst);
		INC_SIZE(4 + 3);
		/* Set low register to conditional flag. */
		*inst++ = (reg_map[TMP_REG1] <= 7) ? REX : REX_B;
		*inst++ = GROUP_0F;
		*inst++ = cond_set;
		*inst++ = MOD_REG | reg_lmap[TMP_REG1];
		*inst++ = REX | (reg_map[TMP_REG1] <= 7 ? 0 : REX_R) | (reg_map[dst] <= 7 ? 0 : REX_B);
		*inst++ = OR_rm8_r8;
		*inst++ = MOD_REG | (reg_lmap[TMP_REG1] << 3) | reg_lmap[dst];
		return SLJIT_SUCCESS;
	}

	reg = (op == SLJIT_MOV && FAST_IS_REG(dst)) ? dst : TMP_REG1;

	inst = (sljit_ub*)ensure_buf(compiler, 1 + 4 + 4);
	FAIL_IF(!inst);
	INC_SIZE(4 + 4);
	/* Set low register to conditional flag. */
	*inst++ = (reg_map[reg] <= 7) ? REX : REX_B;
	*inst++ = GROUP_0F;
	*inst++ = cond_set;
	*inst++ = MOD_REG | reg_lmap[reg];
	*inst++ = REX_W | (reg_map[reg] <= 7 ? 0 : (REX_B | REX_R));
	*inst++ = GROUP_0F;
	*inst++ = MOVZX_r_rm8;
	*inst = MOD_REG | (reg_lmap[reg] << 3) | reg_lmap[reg];

	if (reg != TMP_REG1)
		return SLJIT_SUCCESS;

	if (GET_OPCODE(op) < SLJIT_ADD) {
		compiler->mode32 = GET_OPCODE(op) != SLJIT_MOV;
		return emit_mov(compiler, dst, dstw, TMP_REG1, 0);
	}
#if (defined SLJIT_VERBOSE && SLJIT_VERBOSE) || (defined SLJIT_DEBUG && SLJIT_DEBUG)
	compiler->skip_checks = 1;
#endif
	return sljit_emit_op2(compiler, op, dst, dstw, dst, dstw, TMP_REG1, 0);
#else /* SLJIT_CONFIG_X86_64 */
	if (GET_OPCODE(op) < SLJIT_ADD && FAST_IS_REG(dst)) {
		if (reg_map[dst] <= 4) {
			/* Low byte is accessible. */
			inst = (sljit_ub*)ensure_buf(compiler, 1 + 3 + 3);
			FAIL_IF(!inst);
			INC_SIZE(3 + 3);
			/* Set low byte to conditional flag. */
			*inst++ = GROUP_0F;
			*inst++ = cond_set;
			*inst++ = MOD_REG | reg_map[dst];

			*inst++ = GROUP_0F;
			*inst++ = MOVZX_r_rm8;
			*inst = MOD_REG | (reg_map[dst] << 3) | reg_map[dst];
			return SLJIT_SUCCESS;
		}

		/* Low byte is not accessible. */
		if (cpu_has_cmov == -1)
			get_cpu_features();

		if (cpu_has_cmov) {
			EMIT_MOV(compiler, TMP_REG1, 0, SLJIT_IMM, 1);
			/* a xor reg, reg operation would overwrite the flags. */
			EMIT_MOV(compiler, dst, 0, SLJIT_IMM, 0);

			inst = (sljit_ub*)ensure_buf(compiler, 1 + 3);
			FAIL_IF(!inst);
			INC_SIZE(3);

			*inst++ = GROUP_0F;
			/* cmovcc = setcc - 0x50. */
			*inst++ = cond_set - 0x50;
			*inst++ = MOD_REG | (reg_map[dst] << 3) | reg_map[TMP_REG1];
			return SLJIT_SUCCESS;
		}

		inst = (sljit_ub*)ensure_buf(compiler, 1 + 1 + 3 + 3 + 1);
		FAIL_IF(!inst);
		INC_SIZE(1 + 3 + 3 + 1);
		*inst++ = XCHG_EAX_r + reg_map[TMP_REG1];
		/* Set al to conditional flag. */
		*inst++ = GROUP_0F;
		*inst++ = cond_set;
		*inst++ = MOD_REG | 0 /* eax */;

		*inst++ = GROUP_0F;
		*inst++ = MOVZX_r_rm8;
		*inst++ = MOD_REG | (reg_map[dst] << 3) | 0 /* eax */;
		*inst++ = XCHG_EAX_r + reg_map[TMP_REG1];
		return SLJIT_SUCCESS;
	}

	if (GET_OPCODE(op) == SLJIT_OR && !GET_ALL_FLAGS(op) && FAST_IS_REG(dst) && dst == src && reg_map[dst] <= 4) {
		SLJIT_COMPILE_ASSERT(reg_map[SLJIT_SCRATCH_REG1] == 0, scratch_reg1_must_be_eax);
		if (dst != SLJIT_SCRATCH_REG1) {
			inst = (sljit_ub*)ensure_buf(compiler, 1 + 1 + 3 + 2 + 1);
			FAIL_IF(!inst);
			INC_SIZE(1 + 3 + 2 + 1);
			/* Set low register to conditional flag. */
			*inst++ = XCHG_EAX_r + reg_map[TMP_REG1];
			*inst++ = GROUP_0F;
			*inst++ = cond_set;
			*inst++ = MOD_REG | 0 /* eax */;
			*inst++ = OR_rm8_r8;
			*inst++ = MOD_REG | (0 /* eax */ << 3) | reg_map[dst];
			*inst++ = XCHG_EAX_r + reg_map[TMP_REG1];
		}
		else {
			inst = (sljit_ub*)ensure_buf(compiler, 1 + 2 + 3 + 2 + 2);
			FAIL_IF(!inst);
			INC_SIZE(2 + 3 + 2 + 2);
			/* Set low register to conditional flag. */
			*inst++ = XCHG_r_rm;
			*inst++ = MOD_REG | (1 /* ecx */ << 3) | reg_map[TMP_REG1];
			*inst++ = GROUP_0F;
			*inst++ = cond_set;
			*inst++ = MOD_REG | 1 /* ecx */;
			*inst++ = OR_rm8_r8;
			*inst++ = MOD_REG | (1 /* ecx */ << 3) | 0 /* eax */;
			*inst++ = XCHG_r_rm;
			*inst++ = MOD_REG | (1 /* ecx */ << 3) | reg_map[TMP_REG1];
		}
		return SLJIT_SUCCESS;
	}

	/* Set TMP_REG1 to the bit. */
	inst = (sljit_ub*)ensure_buf(compiler, 1 + 1 + 3 + 3 + 1);
	FAIL_IF(!inst);
	INC_SIZE(1 + 3 + 3 + 1);
	*inst++ = XCHG_EAX_r + reg_map[TMP_REG1];
	/* Set al to conditional flag. */
	*inst++ = GROUP_0F;
	*inst++ = cond_set;
	*inst++ = MOD_REG | 0 /* eax */;

	*inst++ = GROUP_0F;
	*inst++ = MOVZX_r_rm8;
	*inst++ = MOD_REG | (0 << 3) /* eax */ | 0 /* eax */;

	*inst++ = XCHG_EAX_r + reg_map[TMP_REG1];

	if (GET_OPCODE(op) < SLJIT_ADD)
		return emit_mov(compiler, dst, dstw, TMP_REG1, 0);

#if (defined SLJIT_VERBOSE && SLJIT_VERBOSE) || (defined SLJIT_DEBUG && SLJIT_DEBUG)
	compiler->skip_checks = 1;
#endif
	return sljit_emit_op2(compiler, op, dst_save, dstw_save, dst_save, dstw_save, TMP_REG1, 0);
#endif /* SLJIT_CONFIG_X86_64 */
}

SLJIT_API_FUNC_ATTRIBUTE sljit_si sljit_get_local_base(struct sljit_compiler *compiler, sljit_si dst, sljit_sw dstw, sljit_sw offset)
{
	CHECK_ERROR();
	check_sljit_get_local_base(compiler, dst, dstw, offset);
	ADJUST_LOCAL_OFFSET(dst, dstw);

	CHECK_EXTRA_REGS(dst, dstw, (void)0);

#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
	compiler->mode32 = 0;
#endif

	ADJUST_LOCAL_OFFSET(SLJIT_MEM1(SLJIT_LOCALS_REG), offset);

#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
	if (NOT_HALFWORD(offset)) {
		FAIL_IF(emit_load_imm64(compiler, TMP_REG1, offset));
#if (defined SLJIT_DEBUG && SLJIT_DEBUG)
		SLJIT_ASSERT(emit_lea_binary(compiler, SLJIT_KEEP_FLAGS, dst, dstw, SLJIT_LOCALS_REG, 0, TMP_REG1, 0) != SLJIT_ERR_UNSUPPORTED);
		return compiler->error;
#else
		return emit_lea_binary(compiler, SLJIT_KEEP_FLAGS, dst, dstw, SLJIT_LOCALS_REG, 0, TMP_REG1, 0);
#endif
	}
#endif

	if (offset != 0)
		return emit_lea_binary(compiler, SLJIT_KEEP_FLAGS, dst, dstw, SLJIT_LOCALS_REG, 0, SLJIT_IMM, offset);
	return emit_mov(compiler, dst, dstw, SLJIT_LOCALS_REG, 0);
}

SLJIT_API_FUNC_ATTRIBUTE struct sljit_const* sljit_emit_const(struct sljit_compiler *compiler, sljit_si dst, sljit_sw dstw, sljit_sw init_value)
{
	sljit_ub *inst;
	struct sljit_const *const_;
#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
	sljit_si reg;
#endif

	CHECK_ERROR_PTR();
	check_sljit_emit_const(compiler, dst, dstw, init_value);
	ADJUST_LOCAL_OFFSET(dst, dstw);

	CHECK_EXTRA_REGS(dst, dstw, (void)0);

	const_ = (struct sljit_const*)ensure_abuf(compiler, sizeof(struct sljit_const));
	PTR_FAIL_IF(!const_);
	set_const(const_, compiler);

#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
	compiler->mode32 = 0;
	reg = SLOW_IS_REG(dst) ? dst : TMP_REG1;

	if (emit_load_imm64(compiler, reg, init_value))
		return NULL;
#else
	if (dst == SLJIT_UNUSED)
		dst = TMP_REG1;

	if (emit_mov(compiler, dst, dstw, SLJIT_IMM, init_value))
		return NULL;
#endif

	inst = (sljit_ub*)ensure_buf(compiler, 2);
	PTR_FAIL_IF(!inst);

	*inst++ = 0;
	*inst++ = 1;

#if (defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64)
	if (dst & SLJIT_MEM)
		if (emit_mov(compiler, dst, dstw, TMP_REG1, 0))
			return NULL;
#endif

	return const_;
}

SLJIT_API_FUNC_ATTRIBUTE void sljit_set_jump_addr(sljit_uw addr, sljit_uw new_addr)
{
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
	*(sljit_sw*)addr = new_addr - (addr + 4);
#else
	*(sljit_uw*)addr = new_addr;
#endif
}

SLJIT_API_FUNC_ATTRIBUTE void sljit_set_const(sljit_uw addr, sljit_sw new_constant)
{
	*(sljit_sw*)addr = new_constant;
}
