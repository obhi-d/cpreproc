
#pragma once
#include <cstdint>
#include <array>
#include <ppr_common.hpp>

namespace ppr
{

enum class token_type : std::uint32_t
{
  ty_eof = 0,
  ty_preprocessor,
  ty_integer,
  ty_hex_integer,
  ty_real_number,
  ty_char,
  ty_newline,
  ty_operator,
  ty_braces,
  ty_bracket,
  ty_string,
  ty_sl_comment,
  ty_blk_comment,
  ty_keyword_ident
};

enum class preprocessor_type : std::uint16_t
{
  pp_define,
  pp_if,
  pp_ifdef,
  pp_else,
  pp_elif,
  pp_endif,
  pp_undef,
  pp_lang_specific,
};

using operator_type = std::array<char, 2>;

struct token
{   
  std::int32_t start       = 0;
  std::int32_t length      = 0;
  std::int32_t whitespaces = 0;
  union
  {
    operator_type     op; // operator type
    preprocessor_type pp_type;
  };
  token_type type = token_type::ty_eof;
};

} // namespace ppr