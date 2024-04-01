#ifndef ZORYA_REFLECTION_H_
#define ZORYA_REFLECTION_H_

#include <cstdint>
#include <cstddef>
#include <utility>
#include <tuple>

#include "ReflectionGenerationUtils.h"
#include "Reflection_auto_generated.h"

#define GET_FIELD_ADDRESS(structName, offset) (((char*)&structName) + offset)


template <typename F, typename S, typename TupleType, size_t... Idx>
constexpr void foreachfieldWithMetaImpl(F&& f, S& reflectedStruct, TupleType& tuple, std::index_sequence<Idx...>) {
	(f(reflectedStruct, std::get<Idx>(tuple)), ...);
}

template <typename F, typename S, typename TupleType, size_t tupleSize = std::tuple_size_v<TupleType>>
constexpr void foreachfieldWithMeta(F&& f, S& reflectedStruct, TupleType& tuple) {
	foreachfieldWithMetaImpl(f, reflectedStruct, tuple, std::make_index_sequence<tupleSize>());
}

template <typename F, typename S>
constexpr void foreachfield(F&& f, S& reflectedStruct) {
	constexpr auto metaTuple = getMeta<S>();
	foreachfieldWithMeta(f, reflectedStruct, metaTuple);
}


struct ReflectionBase {
	virtual void eval() = 0;
	//virtual std::uint8_t foreachreflectedfield(std::uint8_t(*lambda)(const char* structAddr, auto memMeta)) = 0;
};

template <typename T>
struct ReflectionImplementation : ReflectionBase {
	//std::uint8_t foreachreflectedfield(std::uint8_t(*lambda)(const char* structAddr, const MemberMeta& memMeta)) override {
	//	return foreachfield(impl()->getReflectedStruct(), lambda);
	//}
	virtual void eval() {
		foreachfield([](auto&, auto&) {
			Logger::AddLog(Logger::Channel::TRACE, "Logging");
			}, impl()->reflectedStruct);
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
