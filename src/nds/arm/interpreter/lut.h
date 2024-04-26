#ifndef TWICE_INTERPRETER_LUT_H
#define TWICE_INTERPRETER_LUT_H

#include "common/types.h"

namespace twice {

struct arm_cpu;

typedef void (*arm_instruction)(arm_cpu *cpu);

namespace arm::interpreter::gen {

template <typename T, typename F, std::size_t... Is>
constexpr auto
gen_array(F& f, std::index_sequence<Is...>)
{
	return std::array<T, sizeof...(Is)>{ { f(
			std::integral_constant<std::size_t, Is>{})... } };
}

template <typename T, size_t N, typename F>
constexpr auto
gen_array(F f)
{
	return gen_array<T>(f, std::make_index_sequence<N>{});
}

} // namespace arm::interpreter::gen

} // namespace twice

#endif
