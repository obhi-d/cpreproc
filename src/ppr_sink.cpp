
#include <ppr_sink.hpp>
#include <ppr_transform.hpp>

namespace ppr
{

void sink::filter(token const& t, transform const& tf) 
{
  char op[2] = {};
  bool token_ignored = false;
  switch (t.type)
  {
  case token_type::ty_sl_comment:
    [[fallthrough]];
  case token_type::ty_blk_comment:
    if (!ignore_comments)
      handle(t, tf.svalue(t));
    else
      token_ignored = true;
    break;
  case token_type::ty_operator:
    op[0] = t.op;
    handle(t, tf.svalue(t));
    break;
  case token_type::ty_braces:
    [[fallthrough]];
  case token_type::ty_bracket:
    op[0] = t.op;
    handle(t, tf.svalue(t));
    break;
  case token_type::ty_eof:
    break;
  case token_type::ty_false:
    handle(t, tf.svalue(t));
    break;
  case token_type::ty_operator2:
    [[fallthrough]];
  case token_type::ty_string:
    [[fallthrough]];
  case token_type::ty_sqstring:
    [[fallthrough]];
  case token_type::ty_real_number:
    [[fallthrough]];
  case token_type::ty_preprocessor:
    [[fallthrough]];
  case token_type::ty_keyword_ident:
    [[fallthrough]];
  case token_type::ty_integer:
    [[fallthrough]];
  case token_type::ty_oct_integer:
    [[fallthrough]];
  case token_type::ty_hex_integer:
    handle(t, tf.svalue(t));
    break;
  case token_type::ty_newline:
  {
    bool allow = true;
    if (max_consequtive_newlines > 0)
    {
      if (last.type != token_type::ty_newline)
        consequtive_newlines = 1;
      else if (consequtive_newlines++ > max_consequtive_newlines)
        allow = false;
    }
    if (allow)
      handle(t, tf.svalue(t));
    break;
  }
  }
  if (!token_ignored)
    last = t;
}
}