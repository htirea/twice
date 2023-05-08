#include <cstdio>

#define WRITE(...) fprintf(f, __VA_ARGS__)

static void
generate_arm_lut(FILE *f)
{
	WRITE("#include \"nds/arm/interpreter/lut.h\"\n");
	WRITE("#include \"nds/arm/interpreter/arm_inst.h\"\n\n");
	WRITE("namespace twice {\n\n");

	WRITE("const ArmInstruction arm_inst_lut[4096] = {\n");

	for (unsigned int i = 0; i < 4096; i++) {
		WRITE("\t");

		/* undefined instructions */
		if ((i & 0xFB0) == 0x300) {
			WRITE("arm_undefined");
		} else if ((i & 0xE01) == 0x601) {
			WRITE("arm_undefined");
		}

		/* branch instructions */
		else if ((i & 0xE00) == 0xA00) {
			int L = i >> 8 & 1;
			WRITE("arm_b<%d>", L);
		} else if (i == 0x123) {
			WRITE("arm_blx2");
		} else if (i == 0x121) {
			WRITE("arm_bx");
		}

		/* multiply instructions */
		else if ((i & 0xFCF) == 0x009) {
			int A = i >> 5 & 1;
			int S = i >> 4 & 1;
			WRITE("arm_multiply<%d, %d>", A, S);
		} else if ((i & 0xF8F) == 0x089) {
			int U = i >> 6 & 1;
			int A = i >> 5 & 1;
			int S = i >> 4 & 1;
			WRITE("arm_multiply_long<%d, %d, %d>", U, A, S);
		}

		/* misc arithmetic instructions */
		else if (i == 0x161) {
			WRITE("arm_clz");
		}

		/* status register access instructions */
		else if ((i & 0xFBF) == 0x100) {
			int R = i >> 6 & 1;
			WRITE("arm_mrs<%d>", R);
		} else if ((i & 0xFB0) == 0x320) {
			int R = i >> 6 & 1;
			WRITE("arm_msr<1, %d>", R);
		} else if ((i & 0xFBF) == 0x120) {
			int R = i >> 6 & 1;
			WRITE("arm_msr<0, %d>", R);
		}

		/* exception generating instructions */
		else if ((i & 0xF00) == 0xF00) {
			WRITE("arm_swi");
		} else if (i == 0x127) {
			WRITE("arm_bkpt");
		}

		/* saturated addition and subtraction instructions */
		else if ((i & 0xF9F) == 0x105) {
			int OP = i >> 5 & 3;
			WRITE("arm_sat_add_sub<%d>", OP);
		}

		/* dsp integer multiply and multiply accumulate instructions */
		else if ((i & 0xF99) == 0x108) {
			int OP = i >> 5 & 3;
			int Y = i >> 2 & 1;
			int X = i >> 1 & 1;
			WRITE("arm_dsp_multiply<%d, %d, %d>", OP, Y, X);
		}

		/* semaphore instructions */
		else if ((i & 0xFBF) == 0x109) {
			int B = i >> 6 & 1;
			WRITE("arm_swap<%d>", B);
		}

		/* misc load and store instructions */
		else if ((i & 0xE09) == 0x009 && (i & 0xF) != 0x9) {
			int P = i >> 8 & 1;
			int U = i >> 7 & 1;
			int I = i >> 6 & 1;
			int W = i >> 5 & 1;
			int L = i >> 4 & 1;
			int S = i >> 2 & 1;
			int H = i >> 1 & 1;
			WRITE("arm_misc_dt<%d, %d, %d, %d, %d, %d, %d>", P, U,
					I, W, L, S, H);
		}

		/* load and store word or unsigned byte instructions */
		else if (i >> 10 == 0x1) {
			int R = i >> 9 & 1;
			int P = i >> 8 & 1;
			int U = i >> 7 & 1;
			int B = i >> 6 & 1;
			int W = i >> 5 & 1;
			int L = i >> 4 & 1;
			int SHIFT = i >> 1 & 3;
			if (R == 0) {
				SHIFT = 4;
			}
			WRITE("arm_sdt<%d, %d, %d, %d, %d, %d>", P, U, B, W, L,
					SHIFT);
		}

		/* load and store multiple instructions */
		else if (i >> 9 == 0x4) {
			int P = i >> 8 & 1;
			int U = i >> 7 & 1;
			int S = i >> 6 & 1;
			int W = i >> 5 & 1;
			int L = i >> 4 & 1;
			WRITE("arm_block_dt<%d, %d, %d, %d, %d>", P, U, S, W,
					L);
		}

		/* coprocessor instructions */
		else if ((i & 0xFF0) == 0xC40) {
			WRITE("arm_mcrr");
		} else if ((i & 0xFF0) == 0xC50) {
			WRITE("arm_mrrc");
		} else if ((i & 0xE10) == 0xC10) {
			WRITE("arm_ldc");
		} else if ((i & 0xE10) == 0xC00) {
			WRITE("arm_stc");
		} else if ((i & 0xF01) == 0xE00) {
			WRITE("arm_cdp");
		} else if ((i & 0xF01) == 0xE01) {
			int OP1 = i >> 5 & 7;
			int L = i >> 4 & 1;
			int OP2 = i >> 1 & 7;
			WRITE("arm_cop_reg<%d, %d, %d>", OP1, L, OP2);
		}

		/* data processing instructions */
		else if ((i & 0xE00) == 0x200) {
			int OP = i >> 5 & 0xF;
			int S = i >> 4 & 1;
			int MODE = 0;
			WRITE("arm_alu<%d, %d, %d, %d>", OP, S, 0, MODE);
		} else if ((i & 0xE01) == 0x0) {
			int OP = i >> 5 & 0xF;
			int S = i >> 4 & 1;
			int SHIFT = i >> 1 & 3;
			int MODE = 1;
			WRITE("arm_alu<%d, %d, %d, %d>", OP, S, SHIFT, MODE);
		} else if ((i & 0xE09) == 0x1) {
			int OP = i >> 5 & 0xF;
			int S = i >> 4 & 1;
			int SHIFT = i >> 1 & 3;
			int MODE = 2;
			WRITE("arm_alu<%d, %d, %d, %d>", OP, S, SHIFT, MODE);
		}

		/* everything else */
		else {
			WRITE("arm_undefined");
		}

		WRITE(",\n");
	}

	WRITE("};\n\n");
	WRITE("}\n");
}

static void
generate_thumb_lut(FILE *f)
{
	WRITE("#include \"nds/arm/interpreter/lut.h\"\n");
	WRITE("#include \"nds/arm/interpreter/thumb_inst.h\"\n\n");
	WRITE("namespace twice {\n\n");

	WRITE("const ThumbInstruction thumb_inst_lut[1024] = {\n");

	for (unsigned int i = 0; i < 1024; i++) {
		WRITE("\t");

		/* undefined instructions */
		if (i >> 2 == 0xDE) {
			WRITE("thumb_undefined");
		}

		/* exception generating instructions */
		else if (i >> 2 == 0xDF) {
			WRITE("thumb_swi");
		} else if (i >> 2 == 0xBE) {
			WRITE("thumb_bkpt");
		}

		/* everything else */
		else {
			WRITE("thumb_undefined");
		}

		WRITE(",\n");
	}

	WRITE("};\n\n");
	WRITE("}\n");
}

int
main(int argc, char **argv)
{
	(void)argc;

	FILE *f1 = fopen(argv[1], "w");
	if (!f1) {
		fprintf(stderr, "could not open file: %s\n", argv[1]);
		return 1;
	}

	FILE *f2 = fopen(argv[2], "w");
	if (!f2) {
		fprintf(stderr, "could not open file: %s\n", argv[2]);
		return 1;
	}

	generate_arm_lut(f1);
	generate_thumb_lut(f2);

	fclose(f1);
	fclose(f2);

	return 0;
}
