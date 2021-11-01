

#pragma once

#include <ppr_common.hpp>
#include <ppr_eval_type.hpp>
#include <ppr_tokenizer.hpp>

namespace ppr
{

class PPR_API transform
{
public:
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

    void set_result(ppr::eval_type val)
    {
      result = (bool)val;
    }

    void push_error(ppr::span s, std::string_view err)
    {
      tr.push_error(err, "bison", s.begin);
    }
  };

  friend struct live_eval;

  transform(error_reporter rep) : reporter(std::move(rep)) {}

  using listener = std::function<void(std::string_view)>;
  using visitor = std::function<void(token const&)>;

  void push_source(std::string_view sources, listener reciever);

  bool eval(std::string_view sources);

  // template <typename visitor>
  bool recursive_resolve_internal(token const& it, tokenizer&, visitor);

private:
  token is_defined(tokenizer& tk)
  {
    bool  unexpected = false;
    auto  tok        = tk.get();
    token test;
    if (tok.type == token_type::ty_keyword_ident)
    {
      test = tok;
    }
    else if (tok.type == token_type::ty_bracket && tok.op[0] == '(')
    {
      tok = tk.get();

      if (tok.type == token_type::ty_keyword_ident)
      {
        test = tok;
        tok  = tk.get();
        if (tok.type != token_type::ty_bracket || tok.op[0] != ')')
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
      push_error("unexpected token", tok, tk.get_loc());
      return {};
    }
    else
    {
      test.type = (macros.find(value(test)) != macros.end()) ? token_type::ty_true : token_type::ty_false;
      return test;
    }
  }

  void post(token const& t, listener& l)
  {
    if (t.whitespaces > 0)
    {
      std::string ws(static_cast<std::size_t>(t.whitespaces), ' ');
      l(ws);
    }

    l(value(t));
  }

  std::string_view value(token const& t)
  {
    return std::string_view{sources[t.source_id].data() + t.start, static_cast<std::size_t>(t.length)};
  }

  struct macro
  {
    struct wrap
    {
      token t;
      int   replace = -1;
    };

    vector<token, 4> params;
    vector<wrap, 4>  content;
    bool             is_function = false;
  };

  using macromap = std::unordered_map<std::string_view, macro, ppr::str_hash, ppr::str_equal_test>;

  void read_define(tokenizer&, std::string_view&, macro&, listener& l);

  // template <typename visitor>
  void expand_macro_call(macromap::iterator it, tokenizer&, visitor);

  bool eval(tokenizer&);
  bool eval(ppr::transform::live_eval& tk);
  void push_error(std::string_view s, token const& t, loc const& l)
  {
    reporter(s, "", t, l);
    err_bit = true;
  }

  void push_error(std::string_view s, std::string_view t, loc const& l)
  {
    reporter(s, t, {}, l);
    err_bit = true;
  }

  void undefine(tokenizer&);

  error_reporter              reporter;
  vector<token, 8>            cache;
  vector<std::string_view, 8> sources;
  macromap                    macros;
  std::int32_t                disable_depth    = 0;
  std::int32_t                if_depth         = 0;
  bool                        transform_code   = false;
  bool                        ignore_disabled  = true;
  bool                        err_bit          = false;
  bool                        section_disabled = false;
};
} // namespace ppr
