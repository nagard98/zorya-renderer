#ifndef ZORYA_REFLECTION_H_
#define ZORYA_REFLECTION_H_

#include <cstdint>
#include <cstddef>
#include <utility>

#include "ReflectionGenerationUtils.h"

#define PROPERTY(params)

struct ReflectionBase {
	virtual std::uint8_t foreachreflectedfield(std::uint8_t(*lambda)(const char* structAddr, const MemberMeta& memMeta)) = 0;
};

template <typename T>
struct ReflectionImplementation : ReflectionBase {
	std::uint8_t foreachreflectedfield(std::uint8_t(*lambda)(const char* structAddr, const MemberMeta& memMeta)) override {
		{
			return foreachfield(impl()->getReflectedStruct(), lambda);
		}
	}

	T* impl() {
		return static_cast<T*>(this);
	}
};

template <typename T>
struct ReflectionContainer : ReflectionImplementation<ReflectionContainer<T>> {
	T reflectedStruct;

	T* getReflectedStruct() {
		return &reflectedStruct;
	}
};

#endif
