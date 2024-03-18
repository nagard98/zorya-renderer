#ifndef ZORYA_REFLECTION_H_
#define ZORYA_REFLECTION_H_

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

#define PROPERTY(params)

struct MemberIntermediateMeta {
	char* name;
	char* structName;
	zorya::VAR_REFL_TYPE type;
};

struct MemberMeta {
	const char* name;
	size_t offset;
	zorya::VAR_REFL_TYPE type;
};

#define BUILD_FOREACHFIELD(structName) \
template <typename L> \
void foreachfield(structName* _struct, L&& lambda) { \
	const char* structAddr = (char*)_struct;	\
	for (const MemberMeta& memMeta : structName##_meta) { \
		lambda(structAddr, std::forward<const MemberMeta>(memMeta)); \
	} \
} \

#endif