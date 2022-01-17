
%option reentrant
%option noyywrap
%option prefix="pprtok_"
%option never-interactive
%{

#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <ppr_tokenizer.hpp>

#define YY_READ_BUF_SIZE 512
#define YY_NO_UNISTD_H
#define YY_EXTRA_TYPE ppr::tokenizer*
#define YY_INPUT(buf,result,max_size)       \
	result = yyextra->read(buf, max_size);	\

#define yyterminate()		ctx.end();
#define YY_USER_ACTION  ctx.columns(yyleng);

void* pprtok_alloc   (std::size_t bytes, void* yyscanner);
void* pprtok_realloc (void* ptr, std::size_t bytes, void* yyscanner);
void  pprtok_free    (void* ptr, void* yyscanner);

#ifndef YY_NULLPTR
#  define YY_NULLPTR nullptr
#endif

#define YY_DECL extern ppr::token ppr_tokenize(ppr::tokenizer& ctx, void* yyscanner)

%}


%x pprcode pprcomment pprdirective


blank		           [ \t]*
ident		           [a-zA-Z_][a-zA-Z0-9_]*
integer            [+-]?[1-9]+[0-9]*
oct                [0][0-9]*
zero               "0"
hex                [0][xX][0-9A-F]+ 
intsuffix          [uU]?[lL]?("ll"|"LL")?
decimal            [0-9]*"."[0-9]*
real               {decimal}([eE][+-]?{integer})?
op                 [\+\-\*\/\<\>\!\&\|\~\^\?\:\;\,\.\=\[\]\'\\\#\%]
brace              [\{\}]
bracket            [\(\)]

%%


<<EOF>>                    {
                             return ppr::token();
                           }

<INITIAL,pprcode>{

^{blank}\#                 {
                             ctx.whitespace(yyleng - 1);
                             BEGIN(pprdirective);
                             return ctx.make_op('#', 1);
                           }

{blank}                    {
                             ctx.whitespace(yyleng);
                           }

{ident}                    {
                             return ctx.make_ident(yyleng);
                           }

{zero}                     {
                             return ctx.make_integer(yyleng);
                           }
                           
{integer}{intsuffix}       {
                             return ctx.make_integer(yyleng);
                           }

{hex}{intsuffix}           {
                             return ctx.make_hex_integer(yyleng);
                           }

{oct}{intsuffix}           {
                             return ctx.make_oct_integer(yyleng);
                           }

{real}                     {
                             return ctx.make_real_number(yyleng);
                           }

"++"                       {
                             return ctx.make_op(ppr::operator2_type::op_plusplus, 2);
                           }

"--"                       {
                             return ctx.make_op(ppr::operator2_type::op_minusminus, 2);
                           }

"||"                       {
                             return ctx.make_op(ppr::operator2_type::op_or, 2);
                           }

"&&"                       {
                             return ctx.make_op(ppr::operator2_type::op_and, 2);
                           }

"<<"                       {
                             return ctx.make_op(ppr::operator2_type::op_lshift, 2);
                           }

">>"                       {
                             return ctx.make_op(ppr::operator2_type::op_rshift, 2);
                           }

"=="                       {
                             return ctx.make_op(ppr::operator2_type::op_equals, 2);
                           }

"!="                       {
                             return ctx.make_op(ppr::operator2_type::op_nequals, 2);
                           }

"<="                       {
                             return ctx.make_op(ppr::operator2_type::op_lequal, 2);
                           }

">="                       {
                             return ctx.make_op(ppr::operator2_type::op_gequal, 2);
                           }

"->"                       {
                             return ctx.make_op(ppr::operator2_type::op_accessor, 2);
                           }

"::"                       {
                             return ctx.make_op(ppr::operator2_type::op_scope, 2);
                           }

"##"                       {
                             return ctx.make_op(ppr::operator2_type::op_tokpaste, 2);
                           }

"/*"                       {
                             ctx.start_read_len(yyleng);
                             BEGIN(pprcomment);
                           }

"//"[^\n]*                 {
                             return ctx.make_sl_comment(yyleng);
                           }

{op}                       {
                             return ctx.make_op(yytext[0], 1);
                           }

{brace}                    {
                             return ctx.make_braces(yytext[0]);
                           }

{bracket}                  {
                             return ctx.make_bracket(yytext[0]);
                           }

\"(\\.|[^"\\])*\"          {
                             return ctx.make_string(yyleng);
                           }

\'(\\.|[^'\\])*\'          {
                             return ctx.make_squote_string(yyleng);
                           }

\\\n                       {
                             ctx.skip_commit(2);
                             ctx.lines(1);
                           }

[\r]*\n                    {
                             ctx.skip_commit(yyleng - 1);
                             ctx.lines(1);
                             return ctx.make_newline();
                           }
}

<pprcomment>{

[^*\n]*                    {
                             ctx.read_len(yyleng);
                           }

"*"+[^*/\n]*               {
                             ctx.read_len(yyleng);
                           }

\r                         ctx.read_len(1);

\n											   {
                             ctx.read_len(1);
                             ctx.lines(1);
                           }

"*"+"/"									   {
                             BEGIN(pprcode);
                             return ctx.make_blk_comment(ctx.flush_read_len() + yyleng); 
                           }
}


<pprdirective>{

{blank}                    {
                             ctx.whitespace(yyleng);
                           }

"define"                   {
                             BEGIN(pprcode);
                             return ctx.make_ppr(ppr::preprocessor_type::pp_define, 6);
                           }

"if"                       {
                             BEGIN(pprcode);
                             return ctx.make_ppr(ppr::preprocessor_type::pp_if, 2);
                           }

"ifdef"                    {
                             BEGIN(pprcode);
                             return ctx.make_ppr(ppr::preprocessor_type::pp_ifdef, 5);
                           }

"ifndef"                   {
                             BEGIN(pprcode);
                             return ctx.make_ppr(ppr::preprocessor_type::pp_ifndef, 6);
                           }

"else"                     {
                             BEGIN(pprcode);
                             return ctx.make_ppr(ppr::preprocessor_type::pp_else, 4);
                           }

"elif"                     {
                             BEGIN(pprcode);
                             return ctx.make_ppr(ppr::preprocessor_type::pp_elif, 4);
                           }

"endif"                    {
                             BEGIN(pprcode);
                             return ctx.make_ppr(ppr::preprocessor_type::pp_endif, 5);
                           }

"undef"                    {
                             BEGIN(pprcode);
                             return ctx.make_ppr(ppr::preprocessor_type::pp_undef, 5);
                           }

{ident}                    {
                             BEGIN(pprcode);
                             return ctx.make_ppr(ppr::preprocessor_type::pp_lang_specific, yyleng);
                           }
}

%%

namespace ppr
{

void tokenizer::begin_scan() 
{
  pprtok_lex_init_extra(this, &token_scanner);
  // pprtok_set_debug(true, token_scanner);
  pos = 0;
}

void tokenizer::end_scan() 
{
  pprtok_lex_destroy(token_scanner);
  token_scanner = nullptr;
}

}


