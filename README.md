# PPR
Preprocessor for C like language.

Token paste operator is not supported completely.
It could be supported if token merging was possible, but token was designed to be very lightweight.
One idea would be to generate the token string and store it in a buffer seperately and then merge the tokens with source id set to this buffer.
Might implement if required, for shaders this implementation should be sufficient.
