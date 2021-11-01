#pragma once
#include <ppr_token.hpp>

namespace ppr
{
class transform;
class PPR_API sink
{
public:
  sink(int max_consequitive_empty_lines = 1, bool ignore_all_comments = true)
      : max_consequtive_newlines(max_consequitive_empty_lines), ignore_comments(ignore_all_comments)
  {}

  virtual void error(std::string_view, std::string_view, ppr::token, ppr::loc) = 0;
  virtual void handle(token const& t, std::string_view data)                   = 0;

  void set_ignore_comments(bool i)
  {
    ignore_comments = i;
  }

private:
  friend class transform;
  void filter(token const& t, transform const&);

private:
  token last;
  int   consequtive_newlines     = 0;
  int   max_consequtive_newlines = -1;
  bool  ignore_comments          = true;
};
} // namespace ppr