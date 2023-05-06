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
		WRITE("arm_noop");
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
		WRITE("thumb_noop");
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
