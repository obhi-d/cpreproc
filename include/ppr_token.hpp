
#pragma once
#include <cstdint>
#include <array>
#include "ppr_common.hpp"
#include "ppr_loc.hpp"

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
  ty_raw,
  ty_rtoken,
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

struct rtoken
{
  int         replace = -1;
  std::string value;
  std::int16_t whitespaces = 0;
  union
  {
    operator_type  op = 0; // operator type
    operator2_type op2;
  };
  token_type type = token_type::ty_eof;

  constexpr rtoken() = default;
  constexpr rtoken(token_type ttype, std::string_view sv, std::int16_t ws, int r = -1)
      : type(ttype), value(sv), whitespaces(ws), replace(r), op()
  {}
  constexpr rtoken(token_type ttype, operator_type optype, std::string_view sv, std::int16_t ws, int r = -1)
      : type(ttype), value(sv), whitespaces(ws), replace(r), op(optype)
  {}
  constexpr rtoken(operator_type op, std::int16_t ws)
      : type(token_type::ty_operator), op(op), whitespaces(ws), replace(-1)
  {}
  constexpr rtoken(operator2_type op, std::int16_t ws)
      : type(token_type::ty_operator2), op2(op), whitespaces(ws), replace(-1)
  {}

  std::string_view sspace() const
  {
    return std::string_view{value.c_str(), static_cast<std::size_t>(whitespaces)};
  }

  std::string_view svalue() const
  {
    return std::string_view{value.c_str() + whitespaces, value.length() - whitespaces};
  }

  auto op_type() const
  {
    return op;
  }

  auto op2_type() const
  {
    return op2;
  }
};

using rtoken_ptr = rtoken const*;

struct token_data
{
  std::int32_t start       = 0;
  std::int32_t length      = 0;
  ppr::loc     pos         = {};
  std::int16_t whitespaces = 0;
  union
  {
    operator_type     op; // operator type
    operator2_type    op2;
    preprocessor_type pp_type;
  };
};

struct token
{ 
  union content
  {
    token_data td;
    rtoken_ptr rt;
    std::string const* raw;

    content() : td{} {}
    content(rtoken_ptr v) : rt{v} {}
    content(std::string const& r) : raw(&r) {}
  };

#ifndef NDEBUG
  std::string_view sym;
#endif
  content    value;
  token_type type = token_type::ty_eof;
  bool       was_disabled = false;

  token() = default;
  token(bool b) : type(b ? token_type::ty_true : token_type::ty_false) {}
  token(rtoken const& rt) : type(token_type::ty_rtoken), value(&rt) {}
  token(std::string const& r) : type(token_type::ty_raw), value(r) {}
    
  auto op_type() const
  {
    return value.td.op;
  }

  auto op2_type() const
  {
    return value.td.op2;
  }
};

} // namespace ppr