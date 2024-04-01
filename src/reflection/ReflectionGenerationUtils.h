#ifndef REFLECTION_GEN_UTILS_H_
#define REFLECTION_GEN_UTILS_H_

#include <cstdint>
#include <cstddef>
#include <utility>
#include <tuple>

namespace zorya {
	enum VAR_REFL_TYPE {
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
		"wchar_t",
		"STRUCT"
	};
}

template <typename T>
struct ReflectionType {
	constexpr static T* castTo(void* valueToCast) {
		return static_cast<T*>(valueToCast);
	}
};

struct MemberIntermediateMeta {
	char* name;
	char* typeAsString;
	char* metaStructBaseName;
	char* actualStructIdentifier;
	zorya::VAR_REFL_TYPE typeEnum;
};

template <typename T>
struct MemberMeta {
	const char* name;
	const size_t offset;
	const zorya::VAR_REFL_TYPE typeEnum;
	const ReflectionType<T> type;
};

template <typename T>
constexpr auto getMeta() {
	return std::make_tuple();
};


//template <typename T>
//std::uint8_t foreachfield(T* _struct, std::uint8_t(*lambda)(const char* structAddr, const MemberMeta& memMeta));

/*

#define BUILD_FOREACHFIELD(structName, metaStructName) \
template <>	\
std::uint8_t foreachfield<structName>(structName* _struct, std::uint8_t (*lambda)(const char* structAddr, const MemberMeta& memMeta));


#define BUILD_FOREACHFIELD3(structName, metaStructName) \
template <> \
std::uint8_t foreachfield<structName>(structName* _struct, std::uint8_t (*lambda)(const char* structAddr, const MemberMeta& memMeta)) { \
	const char* structAddr = (char*)_struct;	\
	std::uint8_t structFlags = 0; \
	for (const MemberMeta& memMeta : metaStructName) { \
		std::uint8_t memberFlag = lambda(structAddr, std::forward<const MemberMeta>(memMeta)); \
		structFlags |= memberFlag;	\
	} \
	return structFlags;	\
} \


#define BUILD_FOREACHFIELD2(structName, metaStructName) \
template <typename L> \
std::uint8_t foreachfield(structName* _struct, L&& lambda) { \
	const char* structAddr = (char*)_struct;	\
	std::uint8_t structFlags = 0; \
	for (const MemberMeta& memMeta : metaStructName) { \
		lambda(structAddr, std::forward<const MemberMeta>(memMeta)); \
		std::uint8_t memberFlag = 0; \
		structFlags |= memberFlag;	\
	} \
	return structFlags;	\
} \

*/

#endif // !REFLECTION_GEN_UTILS_H_


