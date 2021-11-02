
#pragma once
#include <cstdint>
#include <array>
#include <ppr_common.hpp>
#include <ppr_loc.hpp>

namespace ppr
{

enum class token_type : std::int32_t
{
  ty_false = 0,
  ty_true,
  ty_preprocessor,
  ty_integer,
  ty_hex_integer,
  ty_oct_integer,
  ty_real_number,
  ty_newline,
  ty_operator,
  ty_operator2,
  ty_braces,
  ty_bracket,
  ty_string,
  ty_sqstring,
  ty_sl_comment,
  ty_blk_comment,
  ty_keyword_ident,
  ty_eof = -1,
};

enum class preprocessor_type : std::uint8_t
{
  pp_define,
  pp_if,
  pp_ifdef,
  pp_ifndef,
  pp_else,
  pp_elif,
  pp_endif,
  pp_undef,
  pp_lang_specific,
};

using operator_type = char;
constexpr static std::uint16_t get_opcode(char const a, char const b)
{
  return static_cast<std::uint16_t>(a) << 8 | static_cast<std::uint16_t>(b);
}

enum class operator2_type : std::uint8_t
{
  op_lshift,
  op_rshift,
  op_lequal,
  op_gequal,
  op_equals,
  op_nequals,
  op_and,
  op_or,
  op_plusplus,
  op_minusminus,
  op_accessor,
  op_scope,
  op_tokpaste
};

struct token
{   
  std::int32_t start       = 0;
  std::int32_t length      = 0;
  ppr::loc     pos         = {};
  std::int16_t source_id   = 0;
  std::int16_t whitespaces = 0;
  #ifndef NDEBUG
  std::string_view sym;
  #endif
  union
  {
    operator_type     op; // operator type
    operator2_type    op2;
    preprocessor_type pp_type;
  };
  token_type type = token_type::ty_eof;

  token() = default;
  token(bool b) : type(b ? token_type::ty_true : token_type::ty_false) {}
  
  inline bool similar(token const& other) const 
  {
    return type == other.type && length == other.length;
  }


};

} // namespace ppr