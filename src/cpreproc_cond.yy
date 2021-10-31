%language "C++"
%skeleton "lalr1.cc"

%defines
%define api.parser.class {parser_impl}
%define api.namespace {cpreproc}
%define api.prefix {cpreproc_}
%define api.token.constructor
%define api.value.type variant
%define parse.assert assert
%define parse.trace 
%define parse.error verbose

%code requires
{

namespace cpreproc {
  class location;
  class context;
}
#ifndef YY_NULLPTR
#  define YY_NULLPTR nullptr
#endif
#define YY_DECL extern cpreproc::parser_impl::symbol_type cpreproc_lex(cpreproc::context& ctx, void* yyscanner)

}

%define api.location.type {cpreproc::location}
%param { context& ctx }
%lex-param { void* SCANNER_PARAM  }
%locations
%initial-action
{
  @$.source_name = ctx.get_source();
}


%code
{
#include "cpreproc_context.hpp"
#define SCANNER_PARAM ctx.scanner
YY_DECL;
}


%token 
	END 0     "end of file"
	DEFINED   "defined"
	ADD       "+"
	MINUS     "-"
	MUL       "*"
	DIV       "/"
	LSHIFT    "<<"
	RSHIFT    ">>"
	LESSEQ    "<="
	GREATEREQ ">="
	EQUALS    "=="
	NEQUALS   "!="
	AND       "&&"
	OR        "||"
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
	;


%token <std::string>			IDENT
%token <double>						REAL
%token <std::uint64_t>    UINT
%token <std::int64_t>     INT
%token <bool>             BOOL

%type <cpreproc::eval_type> expression

%start input

%%

input :
	| input line

line :
	END
	| expression END { }


expression :
		INT  { $$ = cpreproc::eval_type($1); }
	| UINT  { $$ = cpreproc::eval_type($1); }
	| REAL  { $$ = cpreproc::eval_type($1); }
	| BOOL  { $$ = cpreproc::eval_type($1); }
	| DEFINED LPAREN IDENT RPAREN { $$ = ctx.has_macro($3); }
	| DEFINED IDENT { $$ = ctx.has_macro($2); }
	| IDENT { $$ = ctx.eval_macro($1); }
	| expression ADD expression { $$ = $1 + $2; }
	| expression MINUS expression { $$ = $1 - $2; }
	| expression MUL expression { $$ = $1 * $2; }
	| expression DIV expression { $$ = $1 / $2; }
	| expression LSHIFT expression { $$ = $1 << $2; }
	| expression RSHIFT expression { $$ = $1 >> $2; }
	| expression LESSEQ expression { $$ = ($1 <= $2); }
	| expression GREATEREQ expression { $$ = ($1 >= $2); }
	| expression EQUALS expression { $$ = ($1 == $2); }
	| expression NEQUALS expression { $$ = ($1 != $2); }
	| expression AND expression { $$ = ($1 && $2); }
	| expression OR expression { $$ = ($1 || $2); }
	| expression LESS expression { $$ = ($1 < $2); }
	| expression GREATER expression { $$ = ($1 > $2); }
	| NOT expression { $$ = !($1); }
	| expression BW_AND expression { $$ = ($1 & $2); }
	| expression BW_OR expression { $$ = ($1 | $2); }
	| expression BW_NOT expression { $$ = ~($1); }
	| expression BW_XOR expression { $$ = ($1 ^ $2); }
	| expression COND expression COLON expression { $$ = $1.as_bool() ? $3 : $5; }
	| LPAREN expression RPAREN { $$ = $2; }


%%

namespace cpreproc {

	
void parser_impl::error(location_type const& l,
												std::string const & e) 
{
  ctx.push_error(l, e.c_str());
}

void context::parse(std::string_view eval, location_type l) 
{
	loc_ = l;
	scanner = nullptr;
	data = eval;
	
	begin_cond_scan();

	parser_impl parser(*this);
	parser.set_debug_level(flags_ & cpreproc::impl::debug);
	
	parser.parse();
	
	end_cond_scan();
}

}