#ifndef DOLL_SCRIPT__TYPE
# define DOLL_SCRIPT__TYPE(Name_)
# define DOLL_SCRIPT__UNDEF__TYPE
#endif
#ifndef DOLL_SCRIPT__ABSTRACT_TYPE
# define DOLL_SCRIPT__ABSTRACT_TYPE(Name_)
# define DOLL_SCRIPT__UNDEF__ABSTRACT_TYPE
#endif
#ifndef DOLL_SCRIPT__TYPE_RANGE
# define DOLL_SCRIPT__TYPE_RANGE(Name_,Head_,Tail_)
# define DOLL_SCRIPT__UNDEF__TYPE_RANGE
#endif

// Builtin:
DOLL_SCRIPT__ABSTRACT_TYPE(BuiltinType)

	// Void
	DOLL_SCRIPT__TYPE(BuiltinVoidType)
	// Int1, Int8, Int16, Int32, Int64, IntCPU, IntPtr
	// UInt1, UInt8, UInt16, UInt32, UInt64, UIntCPU, UIntPtr
	DOLL_SCRIPT__TYPE(BuiltinIntType)
	// Float16, Float32, Float64
	DOLL_SCRIPT__TYPE(BuiltinFloatType)
	// Str, MutStr
	DOLL_SCRIPT__TYPE(BuiltinStringType)

		DOLL_SCRIPT__TYPE_RANGE(BuiltinType,BuiltinVoidType,BuiltinStringType)

// Array (e.g., Int32[5])
DOLL_SCRIPT__TYPE(ArrayType)

// Named types:
DOLL_SCRIPT__ABSTRACT_TYPE(NamedType)

	// enum
	DOLL_SCRIPT__TYPE(EnumType)
	// struct
	DOLL_SCRIPT__TYPE(StructType)
	// class
	DOLL_SCRIPT__TYPE(ClassType)

		DOLL_SCRIPT__TYPE_RANGE(NamedType,EnumType,ClassType)

#ifdef DOLL_SCRIPT__UNDEF__TYPE_RANGE
# undef DOLL_SCRIPT__UNDEF__TYPE_RANGE
# undef DOLL_SCRIPT__TYPE_RANGE
#endif

#ifdef DOLL_SCRIPT__UNDEF__ABSTRACT_TYPE
# undef DOLL_SCRIPT__UNDEF__ABSTRACT_TYPE
# undef DOLL_SCRIPT__ABSTRACT_TYPE
#endif

#ifdef DOLL_SCRIPT__UNDEF__TYPE
# undef DOLL_SCRIPT__UNDEF__TYPE
# undef DOLL_SCRIPT__TYPE
#endif
