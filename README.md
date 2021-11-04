# PPR
Preprocessor for C like language.

This project is under active development and is not matured yet. 
The project is header only.

# Bulding

The project is header only. From the source directory run:


> mkdir ../build &&  cd ../build
> cmake ../ -G Ninja
> cmake --build . --target install --config Release

If you have bison/flex installed, you can generate the parsers from sources if :
- you set these cmake vars:
> PPR_BISON:STRING - path to bison executable
> PPR_FLEX:STRING - path to flex executable
> PPR_USE_PRE_GENERATED_PARSERS:OFF - set to FALSE to avoid copying the pre-generated code

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