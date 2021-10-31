%option reentrant
%option noyywrap
%option prefix="cpreproc_"
%option never-interactive
%option noyyalloc
%option noyyrealloc
%option noyyfree

%{

#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <cpreproc_context.hpp>

#define YY_NO_UNISTD_H
#define YY_EXTRA_TYPE cpreproc::context*
#define YY_INPUT(buf,result,max_size)       \
	result = yyextra->read(buf, max_size);	\
	assert(result <= max_size);			    \
	assert(buf[result] == 0);				\

void* cpreproc_alloc   (std::size_t bytes, void* yyscanner);
void* cpreproc_realloc (void* ptr, std::size_t bytes, void* yyscanner);
void  cpreproc_free    (void* ptr, void* yyscanner);

#define TokenString  std::string(yytext, (std::uint32_t)yyleng))       

%}

%%

blank [ \t]*
ident [a-zA-Z_][a-zA-Z0-9_]*
integer [+-]?[0-9]+
hex     [0][xX][0-9A-F]+ 
decimal [0-9]*"."[0-9]*
real    {decimal}([eE][+-]?{integer})?

<INITIAL>{

{ident}         return cpreproc::parser_impl::make_IDENT(TokenString, ctx.loc());

{integer}       {
  if (yytext[0] == '-')
  {
    std::int64_t value;
    std::from_chars(yytext, yytext + (std::uint32_t)yyleng, value);
    return cpreproc::parser_impl::make_INT(value, ctx.loc());
  }
  else
  {
    std::uint64_t value;
    std::from_chars(yytext, yytext + (std::uint32_t)yyleng, value);
    return cpreproc::parser_impl::make_UINT(value, ctx.loc());
  }
}

{hex}       {
  std::uint64_t value;
  std::from_chars(yytext, yytext + (std::uint32_t)yyleng, value, 16);
  return cpreproc::parser_impl::make_UINT(value, ctx.loc());
}

{real}          {
  double value;
  std::from_chars(yytext, yytext + (std::uint32_t)yyleng, value);
  return cpreproc::parser_impl::make_REAL(value, ctx.loc());
}

"true"          return cpreproc::parser_impl::make_BOOL(true, ctx.loc());

"false"         return cpreproc::parser_impl::make_BOOL(false, ctx.loc());

"defined"       return cpreproc::parser_impl::make_DEFINED(ctx.loc());

"+"             return cpreproc::parser_impl::make_ADD(ctx.loc());

"-"             return cpreproc::parser_impl::make_MINUS(ctx.loc());

"*"             return cpreproc::parser_impl::make_MUL(ctx.loc());

"/"             return cpreproc::parser_impl::make_DIV(ctx.loc());

"<<"            return cpreproc::parser_impl::make_LSHIFT(ctx.loc());

">>"            return cpreproc::parser_impl::make_RSHIFT(ctx.loc());

"<="            return cpreproc::parser_impl::make_LESSEQ(ctx.loc());

">="            return cpreproc::parser_impl::make_GREATEREQ(ctx.loc());

"=="            return cpreproc::parser_impl::make_EQUALS(ctx.loc());

"!="            return cpreproc::parser_impl::make_NEQUALS(ctx.loc());

"&&"            return cpreproc::parser_impl::make_AND(ctx.loc());

"||"            return cpreproc::parser_impl::make_OR(ctx.loc());

"<"             return cpreproc::parser_impl::make_LESS(ctx.loc());

">"             return cpreproc::parser_impl::make_GREATER(ctx.loc());

"!"             return cpreproc::parser_impl::make_NOT(ctx.loc());

"&"             return cpreproc::parser_impl::make_BW_AND(ctx.loc());

"|"             return cpreproc::parser_impl::make_BW_OR(ctx.loc());

"~"             return cpreproc::parser_impl::make_BW_NOT(ctx.loc());

"^"             return cpreproc::parser_impl::make_BW_XOR(ctx.loc());

"("             return cpreproc::parser_impl::make_LPAREN(ctx.loc());

")"             return cpreproc::parser_impl::make_RPAREN(ctx.loc());

"?"             return cpreproc::parser_impl::make_COND(ctx.loc());

":"             return cpreproc::parser_impl::make_COLON(ctx.loc());

"\\\n"                            

"\n"            ctx.next_line(); BEGIN(state_code);  return cpreproc::parser_impl::make_EVAL(ctx.loc());

}

%%

namespace cpreproc
{

void context::begin_cond_scan() 
{
  cpreproc_lex_init_extra(this, &cond_scanner);
  cpreproc_set_debug(flags_ & cpreproc::impl::debug, scanner);
  pos = 0;
}

void context::end_cond_scan() 
{
  cpreproc_lex_destroy(cond_scanner);
}

}
