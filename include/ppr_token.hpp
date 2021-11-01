
#pragma once
#include <cstdint>
#include <array>
#include <ppr_common.hpp>

namespace ppr
{

enum class token_type : std::int32_t
{
  ty_false = 0,
  ty_true,
  ty_preprocessor,
  ty_integer,
  ty_hex_integer,
  ty_real_number,
  ty_newline,
  ty_operator,
  ty_braces,
  ty_bracket,
  ty_string,
  ty_sl_comment,
  ty_blk_comment,
  ty_keyword_ident,
  ty_eof = -1,
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
  ppr::loc     pos         = {};
  std::int16_t source_id   = 0;
  std::int16_t whitespaces = 0;
  union
  {
    operator_type     op; // operator type
    preprocessor_type pp_type;
  };
  token_type type = token_type::ty_eof;
  
  inline bool similar(token const& other) const 
  {
    return type == other.type && length == other.length;
  }


};

} // namespace ppr