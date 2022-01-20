
#include "ppr_sink.hpp"
#include "ppr_transform.hpp"

namespace ppr
{
class transform::token_stream
{

public:
  token_stream(tokenizer& tz) : base(&tz) {}
  token_stream() = default;

  token get()
  {
    while (!read.empty())
    {
      auto& b = read.back();
      if (b.read < b.cache.size())
        return token(b.cache.at(b.read++));
      read.pop_back();
    }
    return base ? base->get() : token{};
  }

  token peek()
  {
    while (!read.empty())
    {
      auto& b = read.back();
      if (b.read < b.cache.size())
        return token(b.cache.at(b.read));
      read.pop_back();
    }
    return base ? base->peek() : token{};
  }

  void push_source(rtoken_cache const& cache)
  {
    read.emplace_back(cache);
  }

  void save(rtoken const& t)
  {
    gen = t;
  }

  rtoken& get_saved()
  {
    return gen;
  }

private:
  struct source
  {
    rtoken_cache const& cache;
    std::uint32_t       read = 0;
    std::uint32_t       end  = 0;

    source(rtoken_cache const& rcache) : cache(rcache) {}
  };

  rtoken                 gen;
  ppr::vector<source, 8> read;

  tokenizer* base = nullptr;
};

struct transform::rtoken_generator
{
  rtoken_generator(transform::rtoken_cache const& c) : content(c) {}

  rtoken get()
  {
    return (pos < static_cast<std::int32_t>(content.size())) ? content[(std::size_t)(pos++)] : rtoken{};
  }

  rtoken peek()
  {
    return (pos < static_cast<std::int32_t>(content.size()) - 1) ? content[pos + 1] : rtoken{};
  }

  transform::rtoken_cache const& content;
  std::int32_t                   pos = 0;
  resolve_state                  rs  = resolve_state::rsset;
};

rtoken transform::from(token const& t)
{
  switch (t.type)
  {
  case token_type::ty_operator:

    [[fallthrough]];

  case token_type::ty_operator2:

    [[fallthrough]];

  case token_type::ty_bracket:

    return rtoken(t.type, t.value.td.op, token_string_range(t), t.value.td.whitespaces);

  case token_type::ty_rtoken:

    return *t.value.rt;

  default:
  {
    return rtoken(t.type, token_string_range(t), t.value.td.whitespaces, -1);
  }
  }
}

void transform::do_substitutions(param_substitution const& subs, rtoken_cache const& input, rtoken_cache& output)
{
  for (auto const& rt : input)
  {
    if (rt.replace >= 0)
    {
      auto const& sub = subs[rt.replace];
      for (auto const& s : sub)
        output.emplace_back(from(s));
    }
    else
      output.push_back(rt);
  }
}

std::tuple<token, bool> transform::is_defined(token_stream& tk)
{
  bool unexpected = false;
  auto tok        = tk.get();
  auto test       = tok;
  if (tok.type == token_type::ty_keyword_ident)
  {}
  else if (tok.type == token_type::ty_bracket && tok.op_type() == '(')
  {
    tok = tk.get();

    if (tok.type == token_type::ty_keyword_ident)
    {
      test = tok;
      tok  = tk.get();
      if (tok.type != token_type::ty_bracket || tok.op_type() != ')')
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
    return std::tuple<token, bool>(test, false);
  }
  else
  {
    return std::tuple<token, bool>(test, is_defined(value(test)));
  }
}

void transform::resolve_identifier(token start, std::string_view sv, token_stream& ts)
{
  auto it = macros.find(sv);
  if (it != macros.end())
  {
    if (it->second.is_function)
    {
      expand_macro_call(*this, it, ts);
    }
    else
    {
      token_stream ts{};
      auto const&  content = it->second.content;
      ts.push_source(content);
      resolve_tokens(ts);
    }
  }
  else
  {
    post(start);
  }
}

void transform::resolve_tokens(token_stream& tk, bool single)
{
  resolve_state rs = resolve_state::rsset;
  while (true)
  {
    if (rs == resolve_state::rsresolve)
    {
      auto& rtok = tk.get_saved();
      if (rtok.type == token_type::ty_keyword_ident)
      {
        resolve_identifier(token(rtok), rtok.svalue(), tk);
        if (single)
          return;
      }
      else
      {
        post(token(rtok));
        if (single)
          return;
      }
      rs = resolve_state::rsset;
    }
    token start = tk.get();
    switch (start.type)
    {
    case token_type::ty_blk_comment:
      [[fallthrough]];
    case token_type::ty_sl_comment:
      post(start);
      if (single)
        return;
      break;
    case token_type::ty_newline:
    case token_type::ty_eof:
      return;
    case token_type::ty_rtoken:
    {
      auto const& rr = *start.value.rt;
      if (rs == resolve_state::rsjoin)
      {
        auto& last = tk.get_saved();
        token_paste(last, rr);
        rs = resolve_state::rsresolve;
        break;
      }
      else if (rr.type == token_type::ty_operator2 && rr.op2_type() == operator2_type::op_tokpaste)
      {
        rs = resolve_state::rsjoin;
        break;
      }
      else
      {
        auto next = tk.peek();

        if ((next.type == token_type::ty_rtoken && next.value.rt->op2_type() == operator2_type::op_tokpaste))
        {
          tk.save(rr);
          break;
        }
      }
      switch (rr.type)
      {
      case token_type::ty_keyword_ident:
        resolve_identifier(token(rr), rr.svalue(), tk);
        if (single)
          return;
        break;
      default:
        post(token(rr));
        if (single)
          return;
        break;
      }
    }
    break;
    case token_type::ty_keyword_ident:
    {
      auto sv = value(start);
      if (sv == "defined")
      {
        // asking if this is defined
        auto [tok, result] = is_defined(tk);
        if (!ignore_disabled)
        {
          tok.was_disabled = true;
          post_const(tok);
        }
        post(result);
        if (single)
          return;
      }
      else
      {
        resolve_identifier(start, sv, tk);
        if (single)
          return;
      }
    }
    break;
    default:
      post(start);
      if (single)
        return;
    }
  }
}

void transform::expand_macro_call(transform& tf, macromap::iterator it, token_stream& tk)
{
  auto tok = tk.get();
  while (istype(tok, token_type::ty_newline))
  {
    tok = tk.get();
  }

  if (!istype(tok, token_type::ty_bracket) || !hasop(tok, '('))
  {
    tf.push_error("unexpected during macro call", tok);
    return;
  }

  auto const&                   mdef = it->second;
  transform::token_cache        local_cache;
  transform::param_substitution substitutions;
  bool                          done = false;
  std::uint32_t                 depth = 1;

  while (!err_bit && !done)
  {
    auto          tok   = tk.get();

    switch (type(tok))
    {
    case token_type::ty_eof:

      tf.push_error("unexpected during macro call", tok);
      return;

    case token_type::ty_bracket:

      if (hasop(tok, '('))
      {
        if (!depth++)
          break;
      }
      else if (hasop(tok, ')'))
      {
        if (!--depth)
        {
          if (!mdef.params.empty())
            substitutions.emplace_back(std::move(local_cache));
          done = true;
          break;
        }
      }

      local_cache.push_back(tok);
      break;

    case token_type::ty_operator:

      if (depth==1 && hasop(tok, ','))
      {
        substitutions.emplace_back(std::move(local_cache));
        break;
      }

      [[fallthrough]];
    default:
      // if next or previous was/is token paste
      local_cache.push_back(tok);
    }
  }

  if (mdef.params.empty())
  {
    token_stream ts{};
    auto const&  content = it->second.content;
    ts.push_source(content);
    resolve_tokens(ts);
  }
  else
  {
    if (substitutions.size() != mdef.params.size())
    {
      push_error("mismatch parameter count", tok);
      return;
    }

    rtoken_cache out;
    do_substitutions(substitutions, mdef.content, out);
    token_stream ss{};
    ss.push_source(out);
    resolve_tokens(ss);
  }
}

void transform::token_paste(rtoken& rt, token const& t)
{
  using tt = token_type;
  if (t.type == tt::ty_operator || t.type == tt::ty_operator2 || t.type == tt::ty_string || t.type == tt::ty_sqstring ||
      t.type == tt::ty_newline)
  {
    push_error("invalid token for pasting", t);
    return;
  }
  // typeof rt remains same, if it was int, it will be int etc
  rt.value += value(t);
}

void transform::token_paste(rtoken& rt, rtoken const& t)
{
  using tt = token_type;
  if (t.type == tt::ty_operator || t.type == tt::ty_operator2 || t.type == tt::ty_string || t.type == tt::ty_sqstring ||
      t.type == tt::ty_newline)
  {
    push_error("invalid token for pasting", t);
    return;
  }
  // typeof rt remains same, if it was int, it will be int etc
  rt.value += value(t);
}

void transform::read_macro_fn(token t, tokenizer& tk, macro& m)
{
  while (!err_bit && t.type != token_type::ty_newline)
  {
    switch (t.type)
    {
    case token_type::ty_keyword_ident:
    {

      auto v       = value(t);
      auto it      = std::find(m.params.begin(), m.params.end(), v);
      int  replace = -1;
      if (it != m.params.end())
        replace = static_cast<int>(std::distance(m.params.begin(), it));

      m.content.emplace_back(std::move(from(t)));
      m.content.back().replace = replace;
    }
    break;
    case token_type::ty_operator:
      [[fallthrough]];
    case token_type::ty_operator2:
      [[fallthrough]];
    default:
      m.content.emplace_back(std::move(from(t)));
      break;
    }
    if (!transform_code && !err_bit)
      post(t);
    t = tk.get();
  }
  if (!transform_code && !err_bit)
    post(t);
}

void transform::read_macro_def(token t, tokenizer& tk, macro& m)
{
  bool tp = false;
  while (!err_bit && t.type != token_type::ty_newline)
  {
    if (!tp)
    {
      if (t.type == token_type::ty_operator2 && t.value.td.op2 == operator2_type::op_tokpaste)
      {
        if (m.content.empty())
        {
          push_error("invalid placement of token paste operator", t);
          return;
        }
        tp = true;
      }
    }
    else
    {
      auto& prev = m.content.back();
      token_paste(prev, t);
    }

    if (!transform_code && !err_bit)
      post(t);
    if (!tp)
      m.content.emplace_back(std::move(from(t)));
    t = tk.get();
  }
  if (!transform_code && !err_bit)
    post(t);
}

std::string transform::read_define(tokenizer& tk, macro& m)
{
  std::string name;
  auto        get_tok = [&tk, this](bool print)
  {
    auto t = tk.get();
    if (print)
      post(t);
    return t;
  };

  token tok;
  tok = get_tok(!transform_code);

  if (tok.type != token_type::ty_keyword_ident)
  {
    push_error("expecting a macro name", tok);
    return name;
  }
  else
  {
    name = value(tok);
    tok  = tk.get();
  }

  if (tok.type == token_type::ty_bracket && tok.value.td.op == '(' && tok.value.td.whitespaces == 0)
  {
    if (!transform_code)
      post(tok);

    m.is_function = true;
    bool done     = false;
    while (!err_bit && !done)
    {
      auto tok = get_tok(!transform_code);
      switch (tok.type)
      {
      case token_type::ty_bracket:
        if (tok.value.td.op == ')')
        {
          done = true;
        }
        break;
      case token_type::ty_operator:
        if (tok.value.td.op == ',')
        {}
        else
        {
          push_error("unexpected operator", tok);
          return name;
        }
        break;
      case token_type::ty_keyword_ident:
        m.params.emplace_back(value(tok));
        break;
      default:
        push_error("unexpected token", tok);
        return name;
      }
    }
    if (!err_bit)
      tok = tk.get();
  }

  if (m.is_function)
    read_macro_fn(tok, tk, m);
  else
    read_macro_def(tok, tk, m);
  m.content.shrink_to_fit();
  return name;
}

token transform::undefine(tokenizer& tk)
{
  token tok = tk.get();

  if (tok.type != token_type::ty_keyword_ident)
  {
    push_error("expecting a macro name", tok);
  }
  else
  {
    auto name = value(tok);
    auto it   = macros.find(name);
    if (it != macros.end())
      macros.erase(it);
  }

  return tok;
}

void transform::preprocess(std::string_view source)
{
  tokenizer    tk(source, *last_sink);
  token_stream ts(tk);
  live_eval    le(*this, ts, *last_sink);
  le.record_content = !ignore_disabled;

  content = source;

  token saved;
  while (!err_bit)
  {
    auto tok     = tk.get();
    bool handled = false;
    bool flip    = false;
    switch (tok.type)
    {
    case token_type::ty_eof:
      return;
    case token_type::ty_preprocessor:
    {
      switch (tok.value.td.pp_type)
      {
      case preprocessor_type::pp_define:
        if (!section_disabled)
        {
          macro m;
          if (!transform_code)
          {
            post(saved);
            post(tok);
          }
          auto name = read_define(tk, m);
          if (!err_bit)
            macros.emplace(std::move(name), std::move(m));
          handled = true;
        }
        break;
      case preprocessor_type::pp_ifndef:
        flip = true;
        [[fallthrough]];
      case preprocessor_type::pp_ifdef:
        if_depth++;
        if (!section_disabled)
        {
          auto [t, res]    = is_defined(ts);
          section_disabled = !res;
          if (flip)
            section_disabled = !section_disabled;

#ifndef PPR_DISABLE_RECORD
          if (!ignore_disabled)
          {
            saved.was_disabled = true;
            tok.was_disabled   = true;
            t.was_disabled     = true;
            post_const(saved);
            post_const(tok);
            post_const(t);
          }

#endif
          handled = true;
        }
        else
        {
          disable_depth++;
        }
        break;
      case preprocessor_type::pp_if:
        if_depth++;
        if (!section_disabled)
        {
          auto save        = exchange(&le);
          section_disabled = !eval(le);
          exchange(save);
#ifndef PPR_DISABLE_RECORD
          if (le.record_content && section_disabled)
          {
            post(saved);
            post(tok);
            post(token(le.record));
          }

#endif
          handled = true;
        }
        else
        {
          disable_depth++;
        }
        break;
      case preprocessor_type::pp_elif:
        if (!disable_depth && section_disabled)
        {
          auto save        = exchange(&le);
          section_disabled = !eval(le);
          exchange(save);
#ifndef PPR_DISABLE_RECORD
          if (le.record_content && section_disabled)
          {
            post(saved);
            post(tok);
            post(token(le.record));
          }
#endif
          handled = true;
        }
        else
        {
          section_disabled = true;
        }
        break;
      case preprocessor_type::pp_else:
        if (section_disabled)
        {
          if (!disable_depth)
          {
#ifndef PPR_DISABLE_RECORD
            if (!ignore_disabled)
            {
              post(saved);
              post(tok);
            }
#endif
            section_disabled = false;
            handled          = true;
          }
        }
        else
          section_disabled = true;
        break;
      case preprocessor_type::pp_endif:
        --if_depth;
        if (section_disabled)
        {
          if (!disable_depth)
          {
#ifndef PPR_DISABLE_RECORD
            if (!ignore_disabled)
            {
              post(saved);
              post(tok);
            }
#endif
            section_disabled = false; // unlock
            handled          = true;
          }
          else
            disable_depth--;
        }
        else
        {
#ifndef PPR_DISABLE_RECORD
          if (!ignore_disabled)
          {
            saved.was_disabled = true;
            tok.was_disabled   = true;
            post_const(saved);
            post_const(tok);
          }
#endif
          handled = true;
        }
        break;
      case preprocessor_type::pp_undef:
        if (!section_disabled)
        {
          auto t = undefine(tk);

#ifndef PPR_DISABLE_RECORD
          if (!ignore_disabled)
          {
            saved.was_disabled = true;
            tok.was_disabled   = true;
            t.was_disabled     = true;
            post_const(saved);
            post_const(tok);
            post_const(t);
          }
#endif

          handled = true;
        }
        break;
      }

      if (!handled && (!section_disabled || !ignore_disabled))
      {
        post(saved);
        post(tok);
      }
    }
    break;
    default:
      if (!(tok.type == token_type::ty_operator && tok.value.td.op == '#' &&
            tk.peek().type == token_type::ty_preprocessor))
      {
        if (transform_code && !section_disabled)
        {
          if (tok.type == token_type::ty_keyword_ident)
          {
            resolve_identifier(tok, value(tok), ts);
          }
          else
            post(tok);
        }
        else if ((!section_disabled || !ignore_disabled))
          post(tok);
      }

      saved = tok;
    }
  }
  content = {};
}

bool transform::eval_bool(std::string_view sv)
{
  tokenizer    tk(sv, *last_sink);
  token_stream ts(tk);
  live_eval    le(*this, ts, *last_sink);
  content     = sv;
  auto prev = exchange(&le);
  bool result = (bool)eval(le);
  exchange(prev);
  content     = {};
  return result;
}

std::uint64_t transform::eval_uint(std::string_view sv)
{
  tokenizer    tk(sv, *last_sink);
  token_stream ts(tk);
  live_eval    le(*this, ts, *last_sink);
  content     = sv;
  auto prev   = exchange(&le);
  auto result = eval(le).uval();
  exchange(prev);
  content     = {};
  return result;
}

void transform::push_error(std::string_view s, token const& t)
{
  last_sink->error(s, value(t), t, t.type != token_type::ty_rtoken ? t.value.td.pos : loc{});
  err_bit = true;
}

void transform::push_error(std::string_view s, std::string_view t, loc const& l)
{
  last_sink->error(s, t, {}, l);
  err_bit = true;
}

} // namespace ppr