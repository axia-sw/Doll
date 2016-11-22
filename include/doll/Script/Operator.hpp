#pragma once

#include "../Core/Defs.hpp"

namespace doll { namespace script {

enum class EAssoc {
	Left,
	Right
};
enum class EOpType {
	Unary,
	Binary
};

enum class EBuiltinOp {
	Invalid,
	None,

	Add,
	Sub,
	Mul,
	Div,
	Mod,
	Pow,
	BitSL,
	BitSR,
	BitRL,
	BitRR,
	BitAnd,
	BitOr,
	BitXor,
	BitNot,
	CmpEq,
	CmpNe,
	CmpLt,
	CmpGt,
	CmpLe,
	CmpGe,
	RelAnd,
	RelOr,
	RelXor,
	RelNot,
	Neg,
	Deref,
	Addr,
	FieldRef,

	StrConcat,
	StrRemove,
	StrRepeat,
	StrAddPath
};

enum class ECast {
	Invalid,
	None,

	SignExtend,
	ZeroExtend,
	RealExtend,
	SignTrunc,
	ZeroTrunc,
	RealTrunc,

	SignedIntToFloat,
	UnsignedIntToFloat,
	FloatToSignedInt,
	FloatToUnsignedInt,

	IntToBool,
	FloatToBool,
	PtrToBool,

	Int8ToStr,
	Int16ToStr,
	Int32ToStr,
	Int64ToStr,
	Int128ToStr,
	UInt8ToStr,
	UInt16ToStr,
	UInt32ToStr,
	UInt64ToStr,
	UInt128ToStr,

	Float16ToStr,
	Float32ToStr,
	Float64ToStr,

	UTF8PtrToStr,
	UTF16PtrToStr
};

struct SOperator {
	Str        operatorText;
	S32        precedence;
	EAssoc     assoc;
	EOpType    type;
	EBuiltinOp builtinOp;
	Bool       isAssignment;
};

inline Bool isRelOp( EBuiltinOp op ) {
	switch( op ) {
	case EBuiltinOp::RelAnd:
	case EBuiltinOp::RelOr:
	case EBuiltinOp::RelXor:
	case EBuiltinOp::RelNot:
		return true;

	default:
		break;
	}

	return false;
}
inline Bool isCmpOp( EBuiltinOp op ) {
	switch( op ) {
	case EBuiltinOp::CmpEq:
	case EBuiltinOp::CmpNe:
	case EBuiltinOp::CmpLt:
	case EBuiltinOp::CmpGt:
	case EBuiltinOp::CmpLe:
	case EBuiltinOp::CmpGe:
		return true;

	default:
		break;
	}

	return false;
}

}}
