

#pragma once

#include <ppr_common.hpp>
#include <ppr_tokenizer.hpp>
#include <ppr_sink.hpp>

namespace ppr
{

struct live_eval;
class PPR_API transform
{
public:
  using token_cache = vector<token, 8>;

  friend class sink;
  friend struct live_eval;

  transform(sink& s) : ss(s) {}

  void push_source(std::string_view sources);

  bool eval(std::string_view sources);

  bool recursive_resolve_internal(token const& it, tokenizer&, token_cache&);

  void set_transform_code(bool tc)
  {
    transform_code = tc;
  }

  void set_ignore_disabled(bool ig)
  {
    ignore_disabled = ig;
  }

  void push_error(std::string_view s, token const& t);
  void push_error(std::string_view s, std::string_view t, loc const& l);

private:
  static inline bool is_token_paste(token const& t) 
  {
    return t.type == ppr::token_type::ty_operator2 && t.op2 == operator2_type::op_tokpaste;
  }

  bool is_defined(tokenizer& tk)
  {
    bool  unexpected = false;
    auto  tok        = tk.get();
    token test;
    if (tok.type == token_type::ty_keyword_ident)
    {
      test = tok;
    }
    else if (tok.type == token_type::ty_bracket && tok.op == '(')
    {
      tok = tk.get();

      if (tok.type == token_type::ty_keyword_ident)
      {
        test = tok;
        tok  = tk.get();
        if (tok.type != token_type::ty_bracket || tok.op != ')')
          unexpected = true;
      }
      else
      {
        unexpected = true;
      }
    }
    else
      unexpected = true;
    if (unexpected)
    {
      push_error("unexpected token", tok);
      return false;
    }
    else
    {
      return (macros.find(value(test)) != macros.end());
    }
  }

  inline void post(token const& t)
  {
    ss.filter(t, *this);
  }

  std::string_view value(token const& t) const
  {
    return std::string_view{sources[t.source_id].data() + t.start, static_cast<std::size_t>(t.length)};
  }

  struct macro
  {
    struct rtoken
    {
      token t;
      int   replace = -1;
    };

    vector<token, 4>  params;
    vector<rtoken, 4> content;
    bool              is_function = false;
  };

  using macromap = std::unordered_map<std::string_view, macro, ppr::str_hash, ppr::str_equal_test>;

  void read_define(tokenizer&, std::string_view&, macro&);

  void expand_macro_call(macromap::iterator it, tokenizer&, token_cache&);

  bool eval(tokenizer&);
  bool eval(ppr::live_eval& tk);
  
  void undefine(tokenizer&);

  sink&              ss;
  vector<token, 8>            cache;
  vector<std::string_view, 8> sources;
  macromap                    macros;
  std::int32_t                disable_depth            = 0;
  std::int32_t                if_depth                 = 0;
  bool                        transform_code           = false;
  bool                        ignore_disabled          = true;
  bool                        err_bit                  = false;
  bool                        section_disabled         = false;
};

struct live_eval
{
  transform&        tr;
  std::uint32_t     i = 0;
  vector<token, 8>& expanded;
  bool              result = 0;

  live_eval(transform& r, vector<token, 8>& e) : tr(r), expanded(e) {}

  token get()
  {
    if (i < expanded.size())
      return expanded[i++];
    return {};
  }

  std::string_view value(token const& t)
  {
    return tr.value(t);
  }

  bool error_bit() const
  {
    return tr.err_bit;
  }

  void set_result(bool val)
  {
    result = val;
  }

  void push_error(ppr::span s, std::string_view err)
  {
    tr.push_error(err, "bison", s.begin);
  }
};
} // namespace ppr
