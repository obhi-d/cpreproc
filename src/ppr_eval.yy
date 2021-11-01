%language "C++"
%skeleton "lalr1.cc"

%defines
%define api.parser.class {parser_impl}
%define api.namespace {ppr}
%define api.prefix {ppr_}
%define api.token.constructor
%define api.value.type variant
%define parse.assert 
%define parse.trace 
%define parse.error verbose

%code requires
{
#include "ppr_eval_type.hpp"
#include "ppr_loc.hpp"
#include "ppr_transform.hpp"
#include <charconv>

namespace ppr {
  class transform;
}
#ifndef YY_NULLPTR
#  define YY_NULLPTR nullptr
#endif
#define YY_DECL extern ppr::parser_impl::symbol_type ppr_lex(ppr::live_eval& ctx)

}

%define api.location.type {ppr::span}
%param { ppr::live_eval& ctx }
%locations
%initial-action
{  
}


%code
{
#include "ppr_transform.hpp"
YY_DECL;
}


%token 
	END 0     "end of file"
	DEFINED   "defined"
	ADD       "+"
	MINUS     "-"
	MUL       "*"
	DIV       "/"
	LESS      "<"
	GREATER	  ">"
	NOT       "!"
	BW_AND    "&"
	BW_OR     "|"
	BW_NOT    "~"
	BW_XOR    "^"
	LPAREN    "("
	RPAREN    ")"
	COND      "?"
	COLON     ":"
	LSHIFT    "<<"
	RSHIFT    ">>"
	LESSEQ    "<="
	GREATEREQ ">="
	EQUALS    "=="
	NEQUALS   "!="
	AND       "&&"
	OR        "||"
	NIL      "nil"
	;


%token <std::uint64_t>				UINT
%token <std::int64_t>					INT
%token <bool>									BOOL

%type <ppr::eval_type> expression primary unary factor term bwshift comparison equality bwand bwxor bwor and or ternary 

%start input

%%

input :
	| input line

line :
	END
	| expression END { ctx.set_result((bool)$1); }

expression : ternary { $$ = $1; }

ternary : or { $$ = $1; }
				| or COND or COLON or { $$ = $1 ? $3 : $5; }

or      : and { $$ = $1; }
				| and OR and { $$ = $1 || $3; }

and     : bwor { $$ = $1; }
				| bwor AND bwor { $$ = $1 && $3; }

bwor    : bwxor { $$ = $1; }
				| bwxor BW_OR bwxor { $$ = $1 | $3; }

bwxor   : bwand { $$ = $1; }
				| bwand BW_XOR bwand { $$ = $1 ^ $3; }

bwand   : equality { $$ = $1; }
				| equality BW_AND equality { $$ = $1 & $3; }

equality : comparison { $$ = $1; }
				 | comparison NEQUALS comparison { $$ = ($1 != $3); }
				 | comparison EQUALS comparison { $$ = ($1 == $3); }

comparison : bwshift { $$ = $1; }
           | bwshift GREATER bwshift { $$ = $1 > $3; }
					 | bwshift GREATEREQ bwshift { $$ = $1 >= $3; }
					 | bwshift LESS bwshift { $$ = $1 > $3; }
					 | bwshift LESSEQ bwshift { $$ = $1 >= $3; }

bwshift : term { $$ = $1; }
				| term LSHIFT term { $$ = $1 << $3; }
				| term RSHIFT term { $$ = $1 >> $3; }

term : factor { $$ = $1; }
			| factor ADD factor { $$ = $1 + $3; }
			| factor MINUS factor { $$ = $1 + $3; }

factor : unary { $$ = $1; }
			| unary MUL unary { $$ = $1 * $3; }
			| unary DIV unary { $$ = $1 / $3; }

unary : MINUS unary { $$ = -$2; }
			|	NOT unary { $$ = !$2; }
			|	BW_NOT unary { $$ = ~$2; }
			| primary


primary : INT { $$ = ppr::eval_type{$1}; }
			| UINT { $$ = ppr::eval_type{$1}; }
			| BOOL { $$ = ppr::eval_type{$1}; }
			| NIL { $$ = ppr::eval_type{}; }
			| LPAREN expression RPAREN { $$ = $2; }

%%

namespace ppr {

	
void parser_impl::error(location_type const& l,
												std::string const & e) 
{
  ctx.push_error(l, e);
}

bool transform::eval(ppr::live_eval& eval) 
{
	parser_impl parser(eval);
	//parser.set_debug_level(flags_ & ppr::impl::debug);
	
	parser.parse();
	return eval.result;
}

}


ppr::parser_impl::symbol_type ppr_lex(ppr::live_eval& ctx)
{
	using namespace ppr;
	if(!ctx.error_bit())
	{
		auto tok = ctx.get();
		switch(tok.type)
		{
		case token_type::ty_eof:
			return ppr::parser_impl::make_END(tok.pos);
		case token_type::ty_true:
			return ppr::parser_impl::make_BOOL(true, tok.pos);
		case token_type::ty_false:
			return ppr::parser_impl::make_BOOL(false, tok.pos);
		case token_type::ty_integer:
		{
			auto s = ctx.value(tok);
			if (s[0] == '-')
			{
				std::int64_t value;
				std::from_chars(s.data(), s.data() + s.length(), value);
				return ppr::parser_impl::make_INT(value, tok.pos);
			}
			else
			{
				std::uint64_t value;
				std::from_chars(s.data(), s.data() + s.length(), value);
				return ppr::parser_impl::make_UINT(value, tok.pos);
			}
		}
		case token_type::ty_hex_integer:
		{
			auto s = ctx.value(tok);
			
			std::uint64_t value;
			std::from_chars(s.data(), s.data() + s.length(), value, 16);
			return ppr::parser_impl::make_UINT(value, tok.pos);
		}
		case token_type::ty_oct_integer:
		{
			auto s = ctx.value(tok);
			
			std::uint64_t value;
			std::from_chars(s.data() + 1, s.data() + s.length(), value, 8);
			return ppr::parser_impl::make_UINT(value, tok.pos);
		}
		case token_type::ty_real_number:
		{
			ctx.tr.push_error("float in preprocessor", tok);
			return ppr::parser_impl::make_END(tok.pos);
		}			
		case token_type::ty_bracket:
			if (tok.op == '(')
				return ppr::parser_impl::make_LPAREN(tok.pos);
			else
				return ppr::parser_impl::make_RPAREN(tok.pos);
		case token_type::ty_operator:
			switch(tok.op)
			{
			case      '+':
				return ppr::parser_impl::make_ADD(tok.pos);
			case      '-':
				return ppr::parser_impl::make_MINUS(tok.pos);
			case      '*':
				return ppr::parser_impl::make_MUL(tok.pos);
			case      '/':
				return ppr::parser_impl::make_DIV(tok.pos);
			case      '<':
				return ppr::parser_impl::make_LESS(tok.pos);
			case	  '>':
				return ppr::parser_impl::make_GREATER(tok.pos);
			case       '!':
				return ppr::parser_impl::make_NOT(tok.pos);
			case    '&':
				return ppr::parser_impl::make_BW_AND(tok.pos);
			case     '|':
				return ppr::parser_impl::make_BW_OR(tok.pos);
			case    '~':
				return ppr::parser_impl::make_BW_NOT(tok.pos);
			case    '^':
				return ppr::parser_impl::make_BW_XOR(tok.pos);
			case    '(':
				return ppr::parser_impl::make_LPAREN(tok.pos);
			case    ')':
				return ppr::parser_impl::make_RPAREN(tok.pos);
			case      '?':
				return ppr::parser_impl::make_COND(tok.pos);
			case     ':':
				return ppr::parser_impl::make_COLON(tok.pos);
			default:
				// error
				return ppr::parser_impl::make_END(tok.pos);
			}
		case token_type::ty_operator2:
			switch(tok.op2)
			{
			case operator2_type::op_lshift:
					return ppr::parser_impl::make_LSHIFT(tok.pos);
			case operator2_type::op_rshift:
					return ppr::parser_impl::make_RSHIFT(tok.pos);
			case operator2_type::op_lequal:
					return ppr::parser_impl::make_LESSEQ(tok.pos);
			case operator2_type::op_gequal:
					return ppr::parser_impl::make_GREATEREQ(tok.pos);
			case operator2_type::op_equals:
					return ppr::parser_impl::make_EQUALS(tok.pos);
			case operator2_type::op_nequals:
					return ppr::parser_impl::make_NEQUALS(tok.pos);
			case operator2_type::op_and:
					return ppr::parser_impl::make_AND(tok.pos);
			case operator2_type::op_or:
					return ppr::parser_impl::make_OR(tok.pos);
			default: // error
				  return ppr::parser_impl::make_END(tok.pos);
			}
		default:
			return ppr::parser_impl::make_NIL(tok.pos);
		}
		
	}
	return ppr::parser_impl::make_END({});
}
