
#include <ppr_sink.hpp>
#include <ppr_transform.hpp>

namespace ppr
{

void sink::filter(token const& t, transform const& tf) 
{
  bool token_ignored = false;
  switch (tf.type(t))
  {
  case token_type::ty_eof:
    break;
  case token_type::ty_sl_comment:
    [[fallthrough]];
  case token_type::ty_blk_comment:
    if (!ignore_comments)
      handle(t, tf.wspace_content_pair(t));
    else
      token_ignored = true;
    break;
  default:
    handle(t, tf.wspace_content_pair(t));
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
      handle(t, tf.wspace_content_pair(t));
    break;
  }
  }
  if (!token_ignored)
    last = t;
}
}