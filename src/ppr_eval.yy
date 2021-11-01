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
#define YY_DECL extern ppr::parser_impl::symbol_type ppr_lex(ppr::transform::live_eval& ctx)

}

%define api.location.type {ppr::span}
%param { ppr::transform::live_eval& ctx }
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
	;


%token <double>								REAL
%token <std::uint64_t>				UINT
%token <std::int64_t>					INT
%token <bool>									BOOL

%left NOT
%type <ppr::eval_type> expression

%start input

%%

input :
	| input line

line :
	END
	| expression END { ctx.set_result($1); }


expression :
		INT  { $$ = ppr::eval_type($1); }
	| UINT  { $$ = ppr::eval_type($1); }
	| REAL  { $$ = ppr::eval_type($1); }
	| BOOL  { $$ = ppr::eval_type(static_cast<std::uint64_t>($1)); }
	| LPAREN expression RPAREN { $$ = $2; }
	| BW_NOT expression { $$ = ~($2); }
	| NOT expression { $$ = !($2); }
	| MINUS expression { $$ = -($2); }
	| expression ADD expression { $$ = $1 + $3; }
	| expression MINUS expression { $$ = $1 - $3; }
	| expression MUL expression { $$ = $1 * $3; }
	| expression DIV expression { $$ = $1 / $3; }
	| expression LSHIFT expression { $$ = $1 << $3; }
	| expression RSHIFT expression { $$ = $1 >> $3; }
	| expression LESSEQ expression { $$ = ($1 <= $3); }
	| expression GREATEREQ expression { $$ = ($1 >= $3); }
	| expression EQUALS expression { $$ = ($1 == $3); }
	| expression NEQUALS expression { $$ = ($1 != $3); }
	| expression AND expression { $$ = ($1 && $3); }
	| expression OR expression { $$ = ($1 || $3); }
	| expression LESS expression { $$ = ($1 < $3); }
	| expression GREATER expression { $$ = ($1 > $3); }
	| expression BW_AND expression { $$ = ($1 & $3); }
	| expression BW_OR expression { $$ = ($1 | $3); }
	| expression BW_XOR expression { $$ = ($1 ^ $3); }
	| expression COND expression COLON expression { $$ = $1 ? $3 : $5; }


%%

namespace ppr {

	
void parser_impl::error(location_type const& l,
												std::string const & e) 
{
  ctx.push_error(l, e);
}

bool transform::eval(ppr::transform::live_eval& eval) 
{
	parser_impl parser(eval);
	//parser.set_debug_level(flags_ & ppr::impl::debug);
	
	parser.parse();
	return eval.result;
}

}


ppr::parser_impl::symbol_type ppr_lex(ppr::transform::live_eval& ctx)
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
		case token_type::ty_real_number:
		{
			auto s = ctx.value(tok);
			
			double value;
			std::from_chars(s.data(), s.data() + s.length(), value);
			return ppr::parser_impl::make_REAL(value, tok.pos);
		}			
		case token_type::ty_bracket:
			if (tok.op[0] == '(')
				return ppr::parser_impl::make_LPAREN(tok.pos);
			else
				return ppr::parser_impl::make_RPAREN(tok.pos);
		case token_type::ty_operator:
		{
			if (tok.op[1] == 0)
			{
				switch(tok.op[0])
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
			}
			else
			{

				if(tok.op == operator_type{'<', '<'})
					return ppr::parser_impl::make_LSHIFT(tok.pos);
				else if(tok.op == operator_type{'>', '>'})
					return ppr::parser_impl::make_RSHIFT(tok.pos);
				else if(tok.op == operator_type{'<', '='})
					return ppr::parser_impl::make_LESSEQ(tok.pos);
				else if(tok.op == operator_type{'>', '='})
					return ppr::parser_impl::make_GREATEREQ(tok.pos);
				else if(tok.op == operator_type{'=', '='})
					return ppr::parser_impl::make_EQUALS(tok.pos);
				else if(tok.op == operator_type{'!', '='})
					return ppr::parser_impl::make_NEQUALS(tok.pos);
				else if(tok.op == operator_type{'&', '&'})
					return ppr::parser_impl::make_AND(tok.pos);
				else if(tok.op == operator_type{'|', '|'})
					return ppr::parser_impl::make_OR(tok.pos);
				else // error
				  return ppr::parser_impl::make_END(tok.pos);
			}
			
		}
		default:
			// error
			return ppr::parser_impl::make_END(tok.pos);
		}
		
	}
	return ppr::parser_impl::make_END({});
}
