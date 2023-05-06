#ifndef TWICE_LUT_H
#define TWICE_LUT_H

namespace twice {

struct Arm;

typedef void (*ArmInstruction)(Arm *cpu);
typedef void (*ThumbInstruction)(Arm *cpu);

extern const ArmInstruction arm_inst_lut[4096];
extern const ThumbInstruction thumb_inst_lut[1024];

} // namespace twice

#endif
