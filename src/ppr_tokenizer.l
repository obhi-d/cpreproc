
%option reentrant
%option noyywrap
%option prefix="pprtok_"
%option never-interactive
%{

#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <ppr_context.hpp>

#define YY_READ_BUF_SIZE 512
#define YY_NO_UNISTD_H
#define YY_EXTRA_TYPE ppr::context*
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

#define YY_DECL extern ppr::token ppr_tokenize(ppr::context& ctx, void* yyscanner)

%}


%x pprcode pprcomment pprdirective


blank		           [ \t]*
ident		           [a-zA-Z_][a-zA-Z0-9_]*
integer            [+-]?[0-9]+
hex                [0][xX][0-9A-F]+ 
decimal            [0-9]*"."[0-9]*
real               {decimal}([eE][+-]?{integer})?
op2                "++"|"--"|"||"|"&&"|"<<"|">>"|"=="|"!="|"<="|">="|"->"|"::"
op                 [\+\-\*\/\<\>\!\&\|\~\^\?\:\;\,\.\=\[\]]
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
                             return ctx.make_op({'#', 0}, 1);
                           }

{blank}                    {
                             ctx.whitespace(yyleng);
                           }

{ident}                    {
                             return ctx.make_ident(yyleng);
                           }

{integer}                  {
                             return ctx.make_integer(yyleng);
                           }

{hex}                      {
                             return ctx.make_hex_integer(yyleng);
                           }

{real}                     {
                             return ctx.make_real_number(yyleng);
                           }

{op2}                      {
                             return ctx.make_op({yytext[0], yytext[1]}, 2);
                           }

"/*"                       {
                             ctx.start_read_len();
                             BEGIN(pprcomment);
                           }

"//"[^\n]*                 {
                             return ctx.make_sl_comment(yyleng);
                           }

{op}                       {
                             return ctx.make_op({yytext[0], 0}, 1);
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

\\\n                       {
                             ctx.lines(1);
                           }

\n                         {
                             ctx.lines(1);
                             return ctx.make_newline();
                           }

{integer}{ident}           {
                             ctx.push_error("invalid token", std::string_view(yytext, yyleng));
                           }
}

<pprcomment>{

[^*\n]*                    {
                             ctx.read_len(yyleng);
                           }

"*"+[^*/\n]*               {
                             ctx.read_len(yyleng);
                           }

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
                             ctx.whitespace(yyleng - 6);
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

void context::begin_scan() 
{
  pprtok_lex_init_extra(this, &token_scanner);
  pprtok_set_debug(true, token_scanner);
  pos = 0;
}

void context::end_scan() 
{
  pprtok_lex_destroy(token_scanner);
  token_scanner = nullptr;
}

}


