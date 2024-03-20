#ifndef REFLECTION_GEN_UTILS_H_
#define REFLECTION_GEN_UTILS_H_

#include <cstdint>
#include <cstddef>
#include <utility>

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
		WCHAR
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
		"WCHAR"
	};
}

struct MemberIntermediateMeta {
	char* name;
	char* metaStructBaseName;
	char* actualStructIdentifier;
	zorya::VAR_REFL_TYPE type;
};

struct MemberMeta {
	const char* name;
	size_t offset;
	zorya::VAR_REFL_TYPE type;
};

template <typename T>
std::uint8_t foreachfield(T* _struct, std::uint8_t(*lambda)(const char* structAddr, const MemberMeta& memMeta));


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

#endif // !REFLECTION_GEN_UTILS_H_


