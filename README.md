# PPR
Preprocessor for C like language.

This project is under active development and is not matured yet. 

# Bulding

The project is header only. 
From the source directory run:


    mkdir ../build &&  cd ../build
    cmake ../ -G Ninja
    cmake --build . --target install --config Release

If you have bison/flex installed, you can generate the parsers from sources if you set these cmake vars:

| Variable | Description |
|----------|--------------
| PPR_BISON:STRING | Path to bison executable |
| PPR_FLEX:STRING | Path to flex executable | 
| PPR_USE_PRE_GENERATED_PARSERS:OFF | Set to FALSE to avoid copying the pre-generated code | 

# Usage


Include ppr.h, in one of your object module, define PPR_IMPLEMENT before including ppr.h.

		#define PPR_IMPLEMENT
		#include <ppr.h>

This will work even with unity builds. It does not matter if you include ppr.h many times, but you must define PPR_IMPLEMENT once before including ppr.h.

## Parsing 

Implement a sink class. Handle tokens inside handle method. `data` is a pair of string:
- First string is leading whitespaces if any.
- Second string is the token content. 

        
        class sink_adapter : public ppr::sink
        {
          std::ostream& out;

        public:

          sink_adapter(std::ostream& sout) : out(sout) {}

          void handle(ppr::token const& t, symvalue const& data) override
          {
            if (t.was_disabled ^ last_disabled)
            {
              if (t.was_disabled)
                out << "/* ";
              else
                out << "*/ ";
              last_disabled = t.was_disabled;
            }

            out << data.first << data.second;
          }

          void error(std::string_view s, std::string_view e, ppr::token t, ppr::loc l) override
          {
            out << "error : " << s << " - " << e << "l(" << l.line << ":" << l.column << ")" << std::endl;
          }

        private:
          bool last_disabled = false;
        };


Create an instace of `ppr::transform`, call `ppr::transform::preprocess` on the source string.


        sink_adapter adapter(out);
        ppr::transform ctx(adapter);

        ctx.preprocess(content);    


Check for errors outside sink using.

        if (ctx.error_bit()) do_something();


You can live evaluate an #if condition using the transform object.

        std::string_view condition = "defined (MY_MACRO) && MY_NUMBER > 100";

        if(ctx.eval(condition)) do_something();



## What this project does

- Evaluates #if/ifdef like constructs and removes dead/disabled sections
- Prints #defines while also storing their definition in memory
- Can incrementally process further sources using an instance of ppr::transform 
- Returns C/C++/GLSL/HLSL tokens using the ppr::sink interface for the user for further processing
- Optionally can return disabled tokens (with a boolean set on the token object : was_disabled)
- Has API to live evalute an #if (condition) 

## What it does not do

- Care about all valid preprocessor directives : If it encounters an alien directive, it returns the token to the user
- Analyze code
- Currently it can replace code tokens with macro replacement, this feature is not well tested.
- Error reporting needs fixup (source locations are not always valid).
 
 