#ifndef TWICE_LUT_GEN_TABLES_H
#define TWICE_LUT_GEN_TABLES_H

#include "nds/arm/interpreter/lut.h"

#include "nds/arm/interpreter/arm/alu.h"
#include "nds/arm/interpreter/arm/branch.h"
#include "nds/arm/interpreter/arm/clz.h"
#include "nds/arm/interpreter/arm/cop.h"
#include "nds/arm/interpreter/arm/dsp_math.h"
#include "nds/arm/interpreter/arm/exception.h"
#include "nds/arm/interpreter/arm/load_store.h"
#include "nds/arm/interpreter/arm/load_store_misc.h"
#include "nds/arm/interpreter/arm/load_store_multiple.h"
#include "nds/arm/interpreter/arm/multiply.h"
#include "nds/arm/interpreter/arm/semaphore.h"
#include "nds/arm/interpreter/arm/status_reg.h"
#include "nds/arm/interpreter/thumb/alu.h"
#include "nds/arm/interpreter/thumb/branch.h"
#include "nds/arm/interpreter/thumb/exception.h"
#include "nds/arm/interpreter/thumb/load_store.h"
#include "nds/arm/interpreter/thumb/load_store_multiple.h"

namespace twice {

namespace arm::interpreter::gen {

template <typename CPUT>
constexpr auto gen_arm_lut = [](auto i) -> void (*)(CPUT *) {
	/* undefined instructions */
	if constexpr ((i & 0xFB0) == 0x300) {
		return arm_undefined<CPUT>;
	} else if constexpr ((i & 0xE01) == 0x601) {
		return arm_undefined<CPUT>;
	}

	/* branch instructions */
	else if constexpr ((i & 0xE00) == 0xA00) {
		constexpr int L = i >> 8 & 1;
		return arm_b<CPUT, L>;
	} else if constexpr (i == 0x123) {
		return arm_blx2<CPUT>;
	} else if constexpr (i == 0x121) {
		return arm_bx<CPUT>;
	}

	/* multiply instructions */
	else if constexpr ((i & 0xFCF) == 0x009) {
		constexpr int A = i >> 5 & 1;
		constexpr int S = i >> 4 & 1;
		return arm_multiply<CPUT, A, S>;
	} else if constexpr ((i & 0xF8F) == 0x089) {
		constexpr int U = i >> 6 & 1;
		constexpr int A = i >> 5 & 1;
		constexpr int S = i >> 4 & 1;
		return arm_multiply_long<CPUT, U, A, S>;
	}

	/* misc arithmetic instructions */
	else if constexpr (i == 0x161) {
		return arm_clz<CPUT>;
	}

	/* status register access instructions */
	else if constexpr ((i & 0xFBF) == 0x100) {
		constexpr int R = i >> 6 & 1;
		return arm_mrs<CPUT, R>;
	} else if constexpr ((i & 0xFB0) == 0x320) {
		constexpr int R = i >> 6 & 1;
		return arm_msr<CPUT, 1, R>;
	} else if constexpr ((i & 0xFBF) == 0x120) {
		constexpr int R = i >> 6 & 1;
		return arm_msr<CPUT, 0, R>;
	}

	/* exception generating instructions */
	else if constexpr ((i & 0xF00) == 0xF00) {
		return arm_swi<CPUT>;
	} else if constexpr (i == 0x127) {
		return arm_bkpt;
	}

	/* saturated addition and subtraction instructions */
	else if constexpr ((i & 0xF9F) == 0x105) {
		constexpr int OP = i >> 5 & 3;
		return arm_sat_add_sub<CPUT, OP>;
	}

	/* dsp integer multiply and multiply accumulate instructions */
	else if constexpr ((i & 0xF99) == 0x108) {
		constexpr int OP = i >> 5 & 3;
		constexpr int Y = i >> 2 & 1;
		constexpr int X = i >> 1 & 1;
		return arm_dsp_multiply<CPUT, OP, Y, X>;
	}

	/* semaphore instructions */
	else if constexpr ((i & 0xFBF) == 0x109) {
		constexpr int B = i >> 6 & 1;
		return arm_swap<CPUT, B>;
	}

	/* misc load and store instructions */
	else if constexpr ((i & 0xE09) == 0x009 && (i & 0xF) != 0x9) {
		constexpr int P = i >> 8 & 1;
		constexpr int U = i >> 7 & 1;
		constexpr int I = i >> 6 & 1;
		constexpr int W = i >> 5 & 1;
		constexpr int L = i >> 4 & 1;
		constexpr int S = i >> 2 & 1;
		constexpr int H = i >> 1 & 1;
		return arm_misc_dt<CPUT, P, U, I, W, L, S, H>;
	}

	/* load and store word or unsigned byte instructions */
	else if constexpr (i >> 10 == 0x1) {
		constexpr int R = i >> 9 & 1;
		constexpr int P = i >> 8 & 1;
		constexpr int U = i >> 7 & 1;
		constexpr int B = i >> 6 & 1;
		constexpr int W = i >> 5 & 1;
		constexpr int L = i >> 4 & 1;
		constexpr int SHIFT = R != 0 ? i >> 1 & 3 : 4;
		return arm_sdt<CPUT, P, U, B, W, L, SHIFT>;
	}

	/* load and store multiple instructions */
	else if constexpr (i >> 9 == 0x4) {
		constexpr int P = i >> 8 & 1;
		constexpr int U = i >> 7 & 1;
		constexpr int S = i >> 6 & 1;
		constexpr int W = i >> 5 & 1;
		constexpr int L = i >> 4 & 1;
		return arm_block_dt<CPUT, P, U, S, W, L>;
	}

	/* coprocessor instructions */
	else if constexpr ((i & 0xFF0) == 0xC40) {
		return arm_mcrr;
	} else if constexpr ((i & 0xFF0) == 0xC50) {
		return arm_mrrc;
	} else if constexpr ((i & 0xE10) == 0xC10) {
		return arm_ldc;
	} else if constexpr ((i & 0xE10) == 0xC00) {
		return arm_stc;
	} else if constexpr ((i & 0xF01) == 0xE00) {
		return arm_cdp;
	} else if constexpr ((i & 0xF01) == 0xE01) {
		constexpr int OP1 = i >> 5 & 7;
		constexpr int L = i >> 4 & 1;
		constexpr int OP2 = i >> 1 & 7;
		return arm_cop_reg<CPUT, OP1, L, OP2>;
	}

	/* data processing instructions */
	else if constexpr ((i & 0xE00) == 0x200) {
		constexpr int OP = i >> 5 & 0xF;
		constexpr int S = i >> 4 & 1;
		constexpr int MODE = 0;
		return arm_alu<CPUT, OP, S, 0, MODE>;
	} else if constexpr ((i & 0xE01) == 0x0) {
		constexpr int OP = i >> 5 & 0xF;
		constexpr int S = i >> 4 & 1;
		constexpr int SHIFT = i >> 1 & 3;
		constexpr int MODE = 1;
		return arm_alu<CPUT, OP, S, SHIFT, MODE>;
	} else if constexpr ((i & 0xE09) == 0x1) {
		constexpr int OP = i >> 5 & 0xF;
		constexpr int S = i >> 4 & 1;
		constexpr int SHIFT = i >> 1 & 3;
		constexpr int MODE = 2;
		return arm_alu<CPUT, OP, S, SHIFT, MODE>;
	}

	/* everything else */
	else {
		return arm_undefined<CPUT>;
	}
};

template <typename CPUT>
constexpr auto gen_thumb_lut = [](auto i) -> void (*)(CPUT *) {
	/* undefined instructions */
	if constexpr (i >> 2 == 0xDE) {
		return thumb_undefined<CPUT>;
	}

	/* exception generating instructions */
	else if constexpr (i >> 2 == 0xDF) {
		return thumb_swi<CPUT>;
	} else if constexpr (i >> 2 == 0xBE) {
		return thumb_bkpt;
	}

	/* branch instructions */
	else if constexpr (i >> 6 == 0xD) {
		constexpr int COND = i >> 2 & 0xF;
		return thumb_b1<CPUT, COND>;
	} else if constexpr (i >> 5 == 0x1C) {
		return thumb_b2<CPUT>;
	} else if constexpr (i >> 7 == 0x7) {
		constexpr int H = i >> 5 & 3;
		return thumb_b_pair<CPUT, H>;
	} else if constexpr (i >> 1 == 0x8E) {
		return thumb_bx<CPUT>;
	} else if constexpr (i >> 1 == 0x8F) {
		return thumb_blx2<CPUT>;
	}

	/* data processing instructions */
	else if constexpr (i >> 5 == 0x3) {
		constexpr int OP = i >> 3 & 3;
		constexpr int IMM = i & 7;
		return thumb_alu1_2<CPUT, OP, IMM>;
	} else if constexpr (i >> 7 == 1) {
		constexpr int OP = i >> 5 & 3;
		constexpr int RD = i >> 2 & 7;
		return thumb_alu3<CPUT, OP, RD>;
	} else if constexpr (i >> 7 == 0) {
		constexpr int OP = i >> 5 & 3;
		constexpr int IMM = i & 0x1F;
		return thumb_alu4<CPUT, OP, IMM>;
	} else if constexpr (i >> 4 == 0x10) {
		constexpr int OP = i & 0xF;
		return thumb_alu5<CPUT, OP>;
	} else if constexpr (i >> 6 == 0xA) {
		constexpr int R = i >> 5 & 1;
		constexpr int RD = i >> 2 & 7;
		return thumb_alu6<CPUT, R, RD>;
	} else if constexpr (i >> 2 == 0xB0) {
		constexpr int OP = i >> 1 & 1;
		return thumb_alu7<CPUT, OP>;
	} else if constexpr (i >> 4 == 0x11) {
		constexpr int OP = i >> 2 & 3;
		return thumb_alu8<CPUT, OP>;
	}

	/* load / store instructions */
	else if constexpr (i >> 7 == 0x3 || i >> 6 == 0x8) {
		constexpr int OP = i >> 5 & 0x1F;
		constexpr int OFFSET = i & 0x1F;
		return thumb_load_store_imm<CPUT, OP, OFFSET>;
	} else if constexpr (i >> 6 == 0x5) {
		constexpr int OP = i >> 3;
		constexpr int RM = i & 7;
		return thumb_load_store_reg<CPUT, OP, RM>;
	} else if constexpr (i >> 5 == 0x9) {
		constexpr int RD = i >> 2 & 7;
		return thumb_load_pc_relative<CPUT, RD>;
	} else if constexpr (i >> 6 == 0x9) {
		constexpr int L = i >> 5 & 1;
		constexpr int RD = i >> 2 & 7;
		return thumb_load_store_sp_relative<CPUT, L, RD>;
	}

	/* load / store multiple instructions */
	else if constexpr (i >> 6 == 0xC) {
		constexpr int L = i >> 5 & 1;
		constexpr int RN = i >> 2 & 7;
		return thumb_ldm_stm<CPUT, L, RN>;
	} else if constexpr ((i & 0x3D8) == 0x2D0) {
		constexpr int L = i >> 5 & 1;
		constexpr int R = i >> 2 & 1;
		return thumb_push_pop<CPUT, L, R>;
	}

	/* everything else */
	else {
		return thumb_undefined<CPUT>;
	}
};

} // namespace arm::interpreter::gen

} // namespace twice

#endif
