#ifndef ZORYA_REFLECTION_H_
#define ZORYA_REFLECTION_H_

#include <cstdint>
#include <cstddef>
#include <utility>
#include <tuple>

#include "ReflectionGenerationUtils.h"
#include "Reflection_auto_generated.h"


namespace zorya
{
	#define GET_FIELD_ADDRESS(structName, offset) (((char*)&structName) + offset)

	template <typename F, typename S, typename Tuple_Type, size_t... idx>
	constexpr void for_each_field_with_meta_impl(F&& f, S& reflected_struct, Tuple_Type& tuple, std::index_sequence<idx...>)
	{
		(f(reflected_struct, std::get<idx>(tuple)), ...);
	}

	template <typename F, typename S, typename Tuple_Type, size_t tuple_size = std::tuple_size_v<Tuple_Type>>
	constexpr void for_each_field_with_meta(F&& f, S& reflected_struct, Tuple_Type& tuple)
	{
		for_each_field_with_meta_impl(f, reflected_struct, tuple, std::make_index_sequence<tuple_size>());
	}

	template <typename F, typename S>
	constexpr void for_each_field(F&& f, S& reflected_struct)
	{
		constexpr auto meta_tuple = get_meta<S>();
		for_each_field_with_meta(f, reflected_struct, meta_tuple);
	}


	struct Reflection_Base
	{
		virtual void eval() = 0;
		//virtual std::uint8_t foreachreflectedfield(std::uint8_t(*lambda)(const char* structAddr, auto memMeta)) = 0;
	};

	template <typename T>
	struct Reflection_Implementation : Reflection_Base
	{
		//std::uint8_t foreachreflectedfield(std::uint8_t(*lambda)(const char* structAddr, const MemberMeta& memMeta)) override {
		//	return foreachfield(impl()->getReflectedStruct(), lambda);
		//}
		virtual void eval()
		{
			for_each_field([](auto&, auto&) {
				Logger::add_log(Logger::Channel::TRACE, "Logging");
				}, impl()->reflected_struct);
		}

		T* impl()
		{
			return static_cast<T*>(this);
		}
	};

	template <typename T>
	struct Reflection_Container : Reflection_Implementation<Reflection_Container<T>>
	{
		T reflected_struct;

		T* get_reflected_struct()
		{
			return &reflected_struct;
		}
	};

}
#endif
