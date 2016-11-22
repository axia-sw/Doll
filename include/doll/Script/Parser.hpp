#pragma once

#include "../Core/Defs.hpp"
#include "Lexer.hpp"

namespace doll { namespace script {

class StmtSeq;
class Expr;
class ExprSeq;

struct SToken;
struct SOperator;

class CParser {
	CLexer &m_lexer;

	Expr *parseTerminal();
	Expr *parseNameTerminal();
	Expr *parseUnaryExpression( TArr<SOperator> ops );
	Expr *parseSubexpression( S32 precedenceLevel, TArr<SOperator> ops );

public:
	CParser( CLexer &lexer )
	: m_lexer( lexer )
	{
	}
	~CParser() {
	}

	Bool parseProgram();
	Bool parseStatement( StmtSeq &dstSeq );
	Expr *parseExpression( Expr *prntExpr = nullptr );
	ExprSeq *parseExpressionList( const SToken *tok = nullptr );

	Bool semant();
	Bool codegen();
};

}}
