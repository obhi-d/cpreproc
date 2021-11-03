

#pragma once

#include <list>
#include <ppr_common.hpp>
#include <ppr_sink.hpp>
#include <ppr_tokenizer.hpp>
namespace ppr
{

/*
 * No string content is owned by transform, when data is pushed it is expected
 * that the memory is managed externally and that the memory remains valid
 * during the course of the lifetime of transform object.
 * Ideally you should custom allocate the data memory and push the data accordingly.
 * Even macro names are expected to be managed externally.
 */
struct live_eval;
class PPR_API transform
{
public:
  using token_cache        = vector<token, 8>;
  using rtoken_cache       = vector<rtoken, 8>;
  using rtoken_cache       = std::vector<rtoken>;
  using param_substitution = vector<token_cache, 4>;

  friend class sink;
  friend struct live_eval;

  transform(sink& s) : last_sink(&s) {}

  void push_source(std::string_view sources);

  bool eval(std::string_view sources);

  void token_paste(rtoken& rt, token const& t);
  void token_paste(rtoken& rt, rtoken const& t);

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
  struct rtoken_generator;

  inline token_type type(token const& t) const
  {
    return (t.type == token_type::ty_rtoken ? t.value.rt->type : t.type);
  }

  inline bool istype(token const& t, token_type tt) const
  {
    return tt == (t.type == token_type::ty_rtoken ? t.value.rt->type : t.type);
  }

  inline bool hasop(token const& t, char op) const
  {
    return op == (t.type == token_type::ty_rtoken ? t.value.rt->op_type() : t.op_type());
  }

  rtoken from(token const&);

  inline std::string_view value(rtoken const& t) const
  {
    return t.svalue();
  }

  inline std::string_view token_string_range(token const& t) const
  {
    return value(t.value.td.start - t.value.td.whitespaces, t.value.td.length + t.value.td.whitespaces);
  }

  inline std::string_view value(token const& t) const
  {
    return t.type == token_type::ty_rtoken ? t.value.rt->svalue() : value(t.value.td.start, t.value.td.length);
  }

  static inline bool is_token_paste(rtoken const& t)
  {
    return t.type == ppr::token_type::ty_operator2 && t.op2 == operator2_type::op_tokpaste;
  }

  bool is_not_defined(std::string_view name) const
  {
    return !is_defined(name);
  }

  bool is_defined(std::string_view name) const
  {
    return (macros.find(name) != macros.end());
  }

  static inline token get(tokenizer& tk)
  {
    return tk.get();
  }

  inline void post(token const& t)
  {
    last_sink->filter(t, *this);
  }

  inline std::string_view value(std::int32_t start, std::int32_t length) const
  {
    return std::string_view{content.data() + start, static_cast<std::size_t>(length)};
  }

  inline auto svalue(token const& t) const
  {
    using spair = std::pair<std::string_view, std::string_view>;
    return t.type != token_type::ty_rtoken
             ? spair{value(t.value.td.start - t.value.td.whitespaces, t.value.td.whitespaces),
                     value(t.value.td.start, t.value.td.length)}
             : spair{t.value.rt->sspace(), t.value.rt->svalue()};
  }

  struct macro
  {
    using rtoken = ppr::rtoken;
    vector<std::string, 4> params;
    rtoken_cache           content;
    bool                   is_function = false;
  };

  using macromap = std::unordered_map<std::string, macro, ppr::str_hash, ppr::str_equal_test>;

  void        read_macro_fn(token start, tokenizer&, macro&);
  void        read_macro_def(token start, tokenizer&, macro&);
  std::string read_define(tokenizer&, macro&);

  sink* exchange(sink* newsink)
  {
    sink* save = last_sink;
    last_sink  = newsink;
    return save;
  }

  class token_stream;

  bool eval(tokenizer&);
  bool eval(ppr::live_eval& tk);

  void undefine(tokenizer&);
  bool is_defined(token_stream& tk);

  void expand_macro_call(transform& tf, macromap::iterator it, token_stream& tcache);

  enum class resolve_state
  {
    rsset,
    rsjoin,
    rsresolve
  };

  void resolve_identifier(token start, std::string_view sv, token_stream&);
  void resolve_tokens(token_stream&, bool single = false);

  void do_substitutions(param_substitution const& subs, rtoken_cache const& input, rtoken_cache& output);

  // temporaries
  token_cache      cache;
  std::string_view content;

  sink* last_sink;

  macromap     macros;
  std::int32_t disable_depth    = 0;
  std::int32_t if_depth         = 0;
  bool         transform_code   = false;
  bool         ignore_disabled  = true;
  bool         err_bit          = false;
  bool         section_disabled = false;
};

struct live_eval : public sink
{
  transform&                          tr;
  transform::token_stream&            ts;
  std::uint32_t                       i = 0;
  std::vector<std::pair<rtoken, loc>> saved;
  bool                                result = 0;
  bool                                stop   = false;
  std::pair<rtoken, loc>              empty;

  live_eval(transform& r, transform::token_stream& s) : tr(r), ts(s) {}

  auto const& get()
  {
    do
    {
      if (i < static_cast<std::uint32_t>(saved.size()))
      {
        auto& ret = saved[i++];
        return ret;
      }
      else
      {
        i = 0;
        saved.clear();
        tr.resolve_tokens(ts, true);
      }
    }
    while (!saved.empty());
    return empty;
  }

  std::string_view value(rtoken const& t)
  {
    return t.svalue();
  }

  bool error_bit() const
  {
    return tr.err_bit;
  }

  void set_result(bool val)
  {
    result = val;
  }

  void handle(token const& t, symvalue const& data)
  {
    if (stop)
      return;
    auto ty = tr.from(t);
    if (ty.type != token_type::ty_newline)
      saved.emplace_back(std::move(ty), (t.type != token_type::ty_rtoken) ? t.value.td.pos : loc{});
    else
      stop = true;
  }
  void push_error(std::string_view err, std::string_view tok, loc pos)
  {
    tr.push_error(err, tok, pos);
  }
  void error(std::string_view, std::string_view, ppr::token, ppr::loc) {}
};
} // namespace ppr
