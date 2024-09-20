#ifndef REFLECTION_GEN_UTILS_H_
#define REFLECTION_GEN_UTILS_H_

#include <cstdint>
#include <cstddef>
#include <utility>
#include <tuple>

namespace zorya
{

	enum class VAR_REFL_TYPE
	{
		NOT_SUPPORTED,
		INT,
		FLOAT,
		UINT8,
		UINT16,
		UINT32,
		UINT64,
		XMFLOAT2,
		XMFLOAT3,
		XMFLOAT4,
		BOOL,
		WCHAR,
		STRUCT
	};


	const char* const VAR_REFL_TYPE_STRING[] = {
		"NOT_SUPPORTED",
		"INT",
		"FLOAT",
		"UINT8",
		"UINT16",
		"UINT32",
		"UINT64",
		"XMFLOAT2",
		"XMFLOAT3",
		"XMFLOAT4",
		"BOOL",
		"WCHAR",
		"STRUCT"
	};

	const char* const REFL_TYPE_STRINGIFIED[] = {
		"void",
		"int",
		"float",
		"int",
		"int",
		"int",
		"int",
		"dx::XMFLOAT2",
		"dx::XMFLOAT3",
		"dx::XMFLOAT4",
		"bool",
		"wchar_t",
		"STRUCT"
	};

}

template <typename T>
struct Reflection_Type
{
	constexpr static T* cast_to(void* value_to_cast)
	{
		return static_cast<T*>(value_to_cast);
	}

};

struct Member_Intermediate_Meta
{
	char* name;
	char* type_as_string;
	char* meta_struct_basename;
	char* actual_struct_identifier;
	zorya::VAR_REFL_TYPE type_enum;
};

template <typename T>
struct Member_Meta
{
	const char* name;
	const size_t offset;
	const zorya::VAR_REFL_TYPE type_enum;
	const Reflection_Type<T> type;
};

template <typename T>
constexpr auto get_meta()
{
	return std::make_tuple();
};


#endif // !REFLECTION_GEN_UTILS_H_


