
%option reentrant
%option noyywrap
%option prefix="cpreproc_"
%option never-interactive
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

#define yyterminate()		ctx.end();
#define YY_USER_ACTION  ctx.add_columns((std::uint32_t)yyleng);

void* cpreproc_alloc   (std::size_t bytes, void* yyscanner);
void* cpreproc_realloc (void* ptr, std::size_t bytes, void* yyscanner);
void  cpreproc_free    (void* ptr, void* yyscanner);


%}


%x preproc 
%x state_code 
%x state_sline_comment_cap 
%x state_comment_cap 
%x state_sline_comment 
%x state_comment 
%x state_macro_call 
%x state_define_content 
%x state_define_params 
%x state_define_next_param 
%x state_ifdef 
%x state_if  
%x state_ignore_code 
%x state_define

blank [ \t]*
ident [a-zA-Z_][a-zA-Z0-9_]*
integer [+-]?[0-9]+
hex     [0][xX][0-9A-F]+ 
decimal [0-9]*"."[0-9]*
real    {decimal}([eE][+-]?{integer})?

%%

if (ctx.ignore_replacements())
{
	ctx.set_capture_code_in_scope(true);
	ctx.save_source_state(state_ignore_code);
	BEGIN(state_ignore_code)
}
else
{
	ctx.save_source_state(state_code);
	BEGIN(state_code)
}

<INITIAL, state_code, state_call_param, state_define_content, state_if>{

^{blank}\# {
	BEGIN(preproc);
}

"/*"	{
	if (ctx.ignore_comments())
	{		
		BEGIN(state_comment);
	}
	else
	{
		BEGIN(state_comment_cap);
		ctx.put("/*", 2);
	}
}

"//" {
	if (ctx.ignore_comments())
		BEGIN(state_sline_comment);
	else
	{
		BEGIN(state_sline_comment_cap);
		ctx.put("//", 2);
	}
}

{ident}		{
	auto macro = ctx.is_macro(std::string_view(name, (std::uint32_t)yyleng));
	if (ctx.is_macro(macro))
	{
		if (ctx.is_function(macro))
		{
			BEGIN(state_call_param);
			ctx.begin_call(macro);
		}
		else
			ctx.put(ctx.replace_with(macro));
	}
	else
		ctx.put(name, (std::uint32_t)yyleng);
}

[\:\+\-\[\]\.\>\<\!\~\*\&\.\\\%\^\|\?\=]					ctx.put(yytext[0]);

"/"[^\*\/]                                        ctx.put(yytext, yyleng);

\"([^\\"]|\\")*\"																	ctx.put(yytext, (std::uint32_t)yyleng);

{integer}                                         ctx.put(yytext, yyleng);

[ \t]*                                            ctx.put(yytext, (std::uint32_t)yyleng);

[^ \t\n]+																					ctx.put(yytext, yyleng);

"\\\n"																						ctx.put("\n", 1);

}


<state_code, state_call_param> {

\n                                                ctx.next_line(true);

}

<state_define_content> {

\n                                ctx.end_macro(); BEGIN(state_code); 

}

<state_if> {

\n                              {

	if (!ctx.pop_eval()); 
	{
			BEGIN(state_ignore_code);
	}
	else if (ctx.ignore_replacements())
	{
		ctx.set_capture_code_in_scope(true);
		BEGIN(state_ignore_code); 
	}
	else
		BEGIN(state_code); 

}

<state_code> {

[\(\)\,]																							 ctx.put(yytext[0]);

}

<state_call_param> {
[\,]																							 ctx.call_next_param();
[\(]                                               ctx.call_push_paren_scope();
[\)]                                               if (!ctx.call_pop_paren_scope()) ctx.end_call();
}

<state_ignore_code> {

^{blank}\# {
	BEGIN(preproc);
}
[^\n]+                                            if (ctx.capture_ignored() || ctx.capture_code_in_scope()) ctx.put(yytext, (std::uint32_t)yyleng);

\n                                                ctx.next_line(!ctx.capture_ignored() || ctx.capture_code_in_scope());
}

<state_preproc> {

{blank}"define"{blank}             {
	if(!ctx.ignore_code()) 
		BEGIN(state_define)
	else
		BEGIN(state_ignore_code)
}

{blank}"ifdef"{blank}              {

	if(!ctx.ignore_code()) 
		BEGIN(state_ifdef); 
	else
		BEGIN(state_ignore_code)
	ctx.enter_if_scope(); 
}

{blank}"if"{blank}                 {

	if(!ctx.ignore_code()) 
		BEGIN(state_if); 
	else
		BEGIN(state_ignore_code)
	ctx.enter_if_scope();  
}

{blank}"elif"{blank}                 {

	bool eval = !ctx.exit_if_scope();
	if(eval && !ctx.ignore_code()) // was last if result true?
		BEGIN(state_if); 
	else
		BEGIN(state_ignore_code)	
	ctx.enter_if_scope(eval);
}

{blank}"else"{blank}                 {

	bool eval = !ctx.exit_if_scope();
	if(eval && !ctx.ignore_code()) // was last if result true?
	{
		if(ctx.ignore_replacements())
		{
			ctx.set_capture_code_in_scope(true);
			BEGIN(state_ignore_code); 
		}
		else
			BEGIN(state_code); 
	}
	else
		BEGIN(state_ignore_code)
	ctx.enter_if_scope(eval);  
}

{blank}"endif"{blank}              ctx.exit_if_scope(); 

}


<state_define> {

{ident}                                 { ctx.begin_macro(yytext, (std::uint32_t)yyleng); BEGIN(state_define_content); }
{ident}"("                              { ctx.begin_macro_fn(yytext, (std::uint32_t)yyleng); BEGIN(state_define_next_param); }

}


<state_define_next_param> { 

{blank}                         
{ident}                        ctx.push_macro_param(yytext, yyleng); BEGIN(state_define_params);

}

<state_define_params> {

","                                BEGIN(state_define_next_param);
")"{blank}                         BEGIN(state_define_content);
{blank}                         

}

<state_ifdef> {

{ident}                        {
		
		if (!ctx.is_defined(std::string_view(yytext, (std::uint32_t)yyleng)))
		{
			BEGIN(state_ignore_code);
		}
}

"\\\n"                            
"\n"                              ctx.next_line(); BEGIN(state_code); 

}


<state_comment> {

[^*\n]*
"*"+[^*/\n]*
\n											ctx.next_line();
"*"+"/"									ctx.save_token(); BEGIN(ctx.last_source_state());

}


<state_comment_cap> {

[^*\n]*                 ctx.put(yytext, yyleng);
"*"+[^*/\n]*            ctx.put(yytext, yyleng);
\n											ctx.put('\n'); ctx.next_line();
"*"+"/"									ctx.put("*/", 2); BEGIN(ctx.last_source_state());

}

<state_sline_comment> {
[^\n]*                 
\n                     BEGIN(ctx.last_source_state());
}

<state_sline_comment_cap> {
[^\n]*                 ctx.put(yytext, yyleng);
\n                     BEGIN(ctx.last_source_state());
}


%%

namespace cpreproc
{

}