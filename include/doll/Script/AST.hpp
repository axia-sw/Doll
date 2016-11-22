#pragma once

#include "../Core/Defs.hpp"
#include "CompilerMemory.hpp"

namespace doll { namespace script {

class Decl;
class Expr;
class Stmt;

class Node: public CompilerObject {
	enum Kind {
		kDecl,
		kExpr,
		kStmt
	};
	Kind m_kind;
	union {
		Decl *decl;
		Expr *expr;
		Stmt *stmt;
	} m_data;

public:
	Node( CCompilerContext &ctx, Decl *decl )
	: CompilerObject( ctx )
	, m_kind( kDecl )
	{
		m_data.decl = decl;
	}
	Node( CCompilerContext &ctx, Expr *expr )
	: CompilerObject( ctx )
	, m_kind( kExpr )
	{
		m_data.expr = expr;
	}
	Node( CCompilerContext &ctx, Stmt *stmt )
	: CompilerObject( ctx )
	, m_kind( kStmt )
	{
		m_data.stmt = stmt;
	}

	Bool isDecl() const { return m_kind == kDecl; }
	Bool isExpr() const { return m_kind == kExpr; }
	Bool isStmt() const { return m_kind == kStmt; }

	const Decl *getAsDecl_const() const {
		return isDecl() ? m_data.decl : nullptr;
	}
	const Expr *getAsExpr_const() const {
		return isExpr() ? m_data.expr : nullptr;
	}
	const Stmt *getAsStmt_const() const {
		return isStmt() ? m_data.stmt : nullptr;
	}

	const Decl *getAsDecl() const { return getAsDecl_const(); }
	const Expr *getAsExpr() const { return getAsExpr_const(); }
	const Stmt *getAsStmt() const { return getAsStmt_const(); }

	Decl *getAsDecl() { return const_cast< Decl * >( getAsDecl_const() ); }
	Expr *getAsExpr() { return const_cast< Expr * >( getAsExpr_const() ); }
	Stmt *getAsStmt() { return const_cast< Stmt * >( getAsStmt_const() ); }

	Bool semant();
	Bool codegen();
};

class Decl: public CompilerObject {
public:
	virtual Bool semant();
	virtual Bool codegen();
};
class Expr: public CompilerObject {
public:
	virtual Bool semant();
	virtual Bool codegen();
};
class Stmt: public CompilerObject {
public:
	virtual Bool semant();
	virtual Bool codegen();
};

inline Bool Node::semant() {
	if( Decl *d = getAsDecl() ) {
		return d->semant();
	} else if( Expr *e = getAsExpr() ) {
		return e->semant();
	} else if( Stmt *s = getAsStmt() ) {
		return s->semant();
	}

	AX_UNREACHABLE();
	return false;
}
inline Bool Node::codegen() {
	if( Decl *d = getAsDecl() ) {
		return d->codegen();
	} else if( Expr *e = getAsExpr() ) {
		return e->codegen();
	} else if( Stmt *s = getAsStmt() ) {
		return s->codegen();
	}

	AX_UNREACHABLE();
	return false;
}

}}
