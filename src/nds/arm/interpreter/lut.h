#ifndef TWICE_INTERPRETER_LUT_H
#define TWICE_INTERPRETER_LUT_H

namespace twice {

struct arm_cpu;

typedef void (*arm_instruction)(arm_cpu *cpu);
typedef void (*thumb_instruction)(arm_cpu *cpu);

extern const arm_instruction arm_inst_lut[4096];
extern const thumb_instruction thumb_inst_lut[1024];

} // namespace twice

#endif
