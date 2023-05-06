#ifndef TWICE_ARM_INST_H
#define TWICE_ARM_INST_H

#include <nds/arm/arm.h>

#include <libtwice/exception.h>

namespace twice {

inline void
arm_noop(Arm *cpu)
{
}

inline void
arm_undefined(Arm *cpu)
{
	throw TwiceError("arm undefined instruction");
}

} // namespace twice

#endif
