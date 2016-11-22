#pragma once

#include "../Core/Defs.hpp"

namespace doll { namespace script {

	class CCompilerContext;

#pragma pack(push,1)
	struct SStringData
	{
		// Hash of the string: Used for both lookup and localization
		// High 32-bits is a CRC32 hash, low 32-bits is a custom hash
		U64 uHash;
		// Offset in binary blob to actual data
		U32 uOffset;
		// Number of bytes pointed to
		U16 cBytes;
		// Flags on this string (e.g., to localize) -- see `ESubtokenString`
		U16 uFlags;
	};
#pragma pack(pop)

	class CProgramData
	{
	friend class CCompilerContext;
	public:
		CProgramData();
		~CProgramData();

		Bool addString( const Str &s );

	private:
		TMutArr<SStringData> m_strings;
		TMutArr<U8>          m_blob;
	};

}}
