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
	MODULO    "%"
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
				| expression COND ternary COLON ternary { $$ = $1 ? $3 : $5; }

or      : and { $$ = $1; }
				| or OR and { $$ = $1 || $3; }

and     : bwor { $$ = $1; }
				| and AND bwor { $$ = $1 && $3; }

bwor    : bwxor { $$ = $1; }
				| bwor BW_OR bwxor { $$ = $1 | $3; }

bwxor   : bwand { $$ = $1; }
				| bwxor BW_XOR bwand { $$ = $1 ^ $3; }

bwand   : equality { $$ = $1; }
				| bwand BW_AND equality { $$ = $1 & $3; }

equality : comparison { $$ = $1; }
				 | equality NEQUALS comparison { $$ = ($1 != $3); }
				 | equality EQUALS comparison { $$ = ($1 == $3); }

comparison : bwshift { $$ = $1; }
           | comparison GREATER bwshift { $$ = $1 > $3; }
					 | comparison GREATEREQ bwshift { $$ = $1 >= $3; }
					 | comparison LESS bwshift { $$ = $1 < $3; }
					 | comparison LESSEQ bwshift { $$ = $1 <= $3; }

bwshift : term { $$ = $1; }
				| bwshift LSHIFT term { $$ = $1 << $3; }
				| bwshift RSHIFT term { $$ = $1 >> $3; }

term : factor { $$ = $1; }
			| term ADD factor { $$ = $1 + $3; }
			| term MINUS factor { $$ = $1 + $3; }

factor : unary { $$ = $1; }
			| factor MUL unary { $$ = $1 * $3; }
			| factor DIV unary { $$ = $1 / $3; }
			| factor MODULO unary  { $$ = $1 % $3; }

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
  ctx.push_error(e, " bison ", l.begin);
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
		auto const& tnp = ctx.get();
		auto const& tok = tnp.first;
		auto const pos = tnp.second;
		switch(tok.type)
		{
		case token_type::ty_eof:
			return ppr::parser_impl::make_END(pos);
		case token_type::ty_true:
			return ppr::parser_impl::make_BOOL(true, pos);
		case token_type::ty_false:
			return ppr::parser_impl::make_BOOL(false, pos);
		case token_type::ty_integer:
		{
			auto s = ctx.value(tok);
			if (s[0] == '-')
			{
				std::int64_t value;
				std::from_chars(s.data(), s.data() + s.length(), value);
				return ppr::parser_impl::make_INT(value, pos);
			}
			else
			{
				std::uint64_t value;
				std::from_chars(s.data(), s.data() + s.length(), value);
				return ppr::parser_impl::make_UINT(value, pos);
			}
		}
		case token_type::ty_hex_integer:
		{
			auto s = ctx.value(tok);
			
			std::uint64_t value;
			std::from_chars(s.data(), s.data() + s.length(), value, 16);
			return ppr::parser_impl::make_UINT(value, pos);
		}
		case token_type::ty_oct_integer:
		{
			auto s = ctx.value(tok);
			
			std::uint64_t value;
			std::from_chars(s.data() + 1, s.data() + s.length(), value, 8);
			return ppr::parser_impl::make_UINT(value, pos);
		}
		case token_type::ty_real_number:
		{
			ctx.push_error("float in preprocessor", ctx.value(tok), pos);
			return ppr::parser_impl::make_END(pos);
		}			
		case token_type::ty_bracket:
			if (tok.op == '(')
				return ppr::parser_impl::make_LPAREN(pos);
			else
				return ppr::parser_impl::make_RPAREN(pos);
		case token_type::ty_operator:
			switch(tok.op)
			{
			case      '+':
				return ppr::parser_impl::make_ADD(pos);
			case      '-':
				return ppr::parser_impl::make_MINUS(pos);
			case      '*':
				return ppr::parser_impl::make_MUL(pos);
			case      '/':
				return ppr::parser_impl::make_DIV(pos);
			case      '<':
				return ppr::parser_impl::make_LESS(pos);
			case	  '>':
				return ppr::parser_impl::make_GREATER(pos);
			case       '!':
				return ppr::parser_impl::make_NOT(pos);
			case    '&':
				return ppr::parser_impl::make_BW_AND(pos);
			case     '|':
				return ppr::parser_impl::make_BW_OR(pos);
			case    '~':
				return ppr::parser_impl::make_BW_NOT(pos);
			case    '^':
				return ppr::parser_impl::make_BW_XOR(pos);
			case    '(':
				return ppr::parser_impl::make_LPAREN(pos);
			case    ')':
				return ppr::parser_impl::make_RPAREN(pos);
			case      '?':
				return ppr::parser_impl::make_COND(pos);
			case     ':':
				return ppr::parser_impl::make_COLON(pos);
			default:
				// error
				return ppr::parser_impl::make_END(pos);
			}
		case token_type::ty_operator2:
			switch(tok.op2)
			{
			case operator2_type::op_lshift:
					return ppr::parser_impl::make_LSHIFT(pos);
			case operator2_type::op_rshift:
					return ppr::parser_impl::make_RSHIFT(pos);
			case operator2_type::op_lequal:
					return ppr::parser_impl::make_LESSEQ(pos);
			case operator2_type::op_gequal:
					return ppr::parser_impl::make_GREATEREQ(pos);
			case operator2_type::op_equals:
					return ppr::parser_impl::make_EQUALS(pos);
			case operator2_type::op_nequals:
					return ppr::parser_impl::make_NEQUALS(pos);
			case operator2_type::op_and:
					return ppr::parser_impl::make_AND(pos);
			case operator2_type::op_or:
					return ppr::parser_impl::make_OR(pos);
			default: // error
				  return ppr::parser_impl::make_END(pos);
			}
		default:
			return ppr::parser_impl::make_NIL(pos);
		}
		
	}
	return ppr::parser_impl::make_END({});
}
