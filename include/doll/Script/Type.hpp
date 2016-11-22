#pragma once

#include "../Core/Defs.hpp"
#include "../Util/Casting.hpp"
#include "CompilerMemory.hpp"
#include "SourceLoc.hpp"

namespace doll { namespace script {

	class Ident;
	class RuntimeConfiguration;

	enum class ETypeKind: U32
	{
#define DOLL_SCRIPT__TYPE(Name_) Name_,
#define DOLL_SCRIPT__TYPE_RANGE(Name_,Head_,Tail_) \
	First##Name_ = Head_,\
	Last##Name_ = Tail_,
#include "Types.def.hpp"
#undef DOLL_SCRIPT__TYPE_RANGE
#undef DOLL_SCRIPT__TYPE
	};

	// Represents any type in the language
	class Type: public CompilerObject
	{
	public:
		// Destructor
		virtual ~Type()
		{
		}

		// Retrieve the kind of type this is
		ETypeKind getKind() const
		{
			return m_kind;
		}

		// Check if this matches the type we expect
		template< typename TDstTy >
		AX_FORCEINLINE Bool is() const
		{
			return TDstTy::isClass( this );
		}
		// Cast this to a given type
		template< typename TDstTy >
		AX_FORCEINLINE TDstTy *as()
		{
			return cast< TDstTy >( this );
		}
		template< typename TDstTy >
		AX_FORCEINLINE const TDstTy *as() const
		{
			return cast< TDstTy >( this );
		}

		// Retrieve this type's identifier
		Ident *getIdent() const
		{
			return m_pIdent;
		}
		// Retrieve the location of this type's declaration
		SourceLoc getLoc() const
		{
			return m_declLoc;
		}

		// Retrieve the size of this type, in bytes (e.g., sizeof(Int32)=4)
		U32 getSizeOf() const
		{
			return m_size;
		}

	protected:
		Type( CCompilerContext &ctx, ETypeKind kind )
		: CompilerObject( ctx )
		, m_kind( kind )
		, m_pIdent( nullptr )
		, m_declLoc()
		, m_size( 0 )
		{
		}

	private:
		const ETypeKind m_kind;
		Ident *         m_pIdent;
		SourceLoc       m_declLoc;
		U32             m_size;

		AX_DELETE_COPYFUNCS( Type );
	};

	// Some built-in type
	class BuiltinType: public Type
	{
	public:
		static Bool isClass( const Type *p )
		{
			return
				p->getKind() >= ETypeKind::FirstBuiltinType &&
				p->getKind() <= ETypeKind::LastBuiltinType;
		}

	protected:
		BuiltinType( CCompilerContext &ctx, ETypeKind kind )
		: Type( ctx, kind )
		{
			AX_ASSERT( kind >= ETypeKind::FirstBuiltinType && kind <= ETypeKind::LastBuiltinType );
		}
	};

	// Built-in empty type
	class BuiltinVoidType: public BuiltinType
	{
	friend class CCompilerContext;
	public:
		static Bool isClass( const Type *p )
		{
			return p->getKind() == ETypeKind::BuiltinVoidType;
		}

	private:
		BuiltinVoidType( CCompilerContext &ctx )
		: BuiltinType( ctx, ETypeKind::BuiltinVoidType )
		{
		}
	};

	enum class EIntSign
	{
		Signed,
		Unsigned
	};
	enum class EIntMode
	{
		Fixed,
		Register,
		Pointer
	};

	// Built-in signed or unsigned integer type of a given width or width range
	class BuiltinIntType: public BuiltinType
	{
	friend class CCompilerContext;
	public:
		// Whether this type is signed
		Bool isSigned() const
		{
			return m_isSigned;
		}
		// Whether this type is fixed-width
		Bool isFixedWidth() const
		{
			return m_minWidth == m_maxWidth;
		}

		// Retrieve the fixed bit-width of this type (asserts isFixedWidth())
		U32 getFixedWidth() const
		{
			AX_ASSERT( isFixedWidth() );
			return m_minWidth;
		}

		// Retrieve the minimum bit-width this type might be
		U32 getMinWidth() const
		{
			return m_minWidth;
		}
		// Retrieve the maximum bit-width this type might be
		U32 getMaxWidth() const
		{
			return m_maxWidth;
		}

		// Check whether this is register-sized
		Bool isRegisterSized() const
		{
			return m_mode == EIntMode::Register;
		}
		// Check whether this is pointer-sized
		Bool isPointerSized() const
		{
			return m_mode == EIntMode::Pointer;
		}

		static Bool isClass( const Type *p )
		{
			return p->getKind() == ETypeKind::BuiltinIntType;
		}

	private:
		Bool     m_isSigned;
		EIntMode m_mode;
		U32      m_minWidth;
		U32      m_maxWidth;

		BuiltinIntType( CCompilerContext &ctx, EIntSign sign, U32 width )
		: BuiltinType( ctx, ETypeKind::BuiltinIntType )
		, m_isSigned( sign == EIntSign::Signed )
		, m_mode( EIntMode::Fixed )
		, m_minWidth( width )
		, m_maxWidth( width )
		{
			AX_ASSERT_MSG( width >= 1, "Minimum integer width is 1-bit" );
		}
		BuiltinIntType( CCompilerContext &ctx, EIntSign sign, EIntMode mode )
		: BuiltinType( ctx, ETypeKind::BuiltinIntType )
		, m_isSigned( sign == EIntSign::Signed )
		, m_mode( mode )
		, m_minWidth( 32 )
		, m_maxWidth( 64 )
		{
			AX_ASSERT_MSG( mode != EIntMode::Fixed, "Mode specified should be `Register` or `Pointer`" );
		}
	};

	// Built-in floating-point type of a given width
	class BuiltinFloatType: public BuiltinType
	{
	friend class CCompilerContext;
	public:
		// Retrieve the bit-width of this type
		U32 getWidth() const
		{
			return m_width;
		}

		static Bool isClass( const Type *p )
		{
			return p->getKind() == ETypeKind::BuiltinFloatType;
		}

	private:
		U32 m_width;
		
		BuiltinFloatType( CCompilerContext &ctx, U32 width )
		: BuiltinType( ctx, ETypeKind::BuiltinFloatType )
		, m_width( width )
		{
			AX_ASSERT_MSG( width >= 16, "Floating-point width must be 16-bits or greater" );
			AX_ASSERT_MSG( ( width & ( width - 1 ) ) == 0, "Floating-point width is not a power of two" );
		}
	};

	enum class EContainerMode
	{
		// Dynamic mutable container
		Dynamic,
		// Fixed-storage mutable container
		Static,
		// Immutable view of a container
		View
	};

	// Built-in string type (e.g., Str, MutStr)
	class BuiltinStringType: public BuiltinType
	{
	friend class CCompilerContext;
	public:
		static Bool isClass( const Type *p )
		{
			return p->getKind() == ETypeKind::BuiltinStringType;
		}
		
		EContainerMode getMode() const
		{
			return m_mode;
		}

	private:
		const EContainerMode m_mode;

		BuiltinStringType( CCompilerContext &ctx, EContainerMode mode )
		: BuiltinType( ctx, ETypeKind::BuiltinStringType )
		, m_mode( mode )
		{
		}
	};

	// Array type
	class ArrayType: public Type
	{
	public:
		ArrayType( CCompilerContext &ctx, Type *pBaseTy )
		: Type( ctx, ETypeKind::ArrayType )
		, m_pBaseTy( pBaseTy )
		{
		}

		Type *getBaseType() const
		{
			return m_pBaseTy;
		}

		static Bool isClass( const Type *p )
		{
			return p->getKind() == ETypeKind::ArrayType;
		}

	private:
		Type *m_pBaseTy;
	};

	// Named type -- any type the user can declare
	class NamedType: public Type
	{
	public:
		Type *getParent() const
		{
			return m_pParent;
		}

		static Bool isClass( const Type *p )
		{
			return
				p->getKind() >= ETypeKind::FirstNamedType &&
				p->getKind() <= ETypeKind::LastNamedType;
		}

	protected:
		NamedType( CCompilerContext &ctx, ETypeKind kind, Type *pParent = nullptr )
		: Type( ctx, kind )
		, m_pParent( pParent )
		{
		}

	private:
		Type *m_pParent;
	};

	// Enumeration type
	class EnumType: public NamedType
	{
	public:
		EnumType( CCompilerContext &ctx, Type *pParent = nullptr )
		: NamedType( ctx, ETypeKind::EnumType, pParent )
		{
		}

		static Bool isClass( const Type *p )
		{
			return p->getKind() == ETypeKind::EnumType;
		}
	};
	// Structure type
	class StructType: public NamedType
	{
	public:
		StructType( CCompilerContext &ctx, Type *pParent = nullptr )
		: NamedType( ctx, ETypeKind::StructType, pParent )
		{
		}

		static Bool isClass( const Type *p )
		{
			return p->getKind() == ETypeKind::StructType;
		}
	};
	// Class type
	class ClassType: public NamedType
	{
	public:
		ClassType( CCompilerContext &ctx, Type *pParent = nullptr )
		: NamedType( ctx, ETypeKind::ClassType, pParent )
		{
		}

		static Bool isClass( const Type *p )
		{
			return p->getKind() == ETypeKind::ClassType;
		}
	};

}}
