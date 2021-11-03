
#include <ppr_sink.hpp>
#include <ppr_transform.hpp>
#include <variant>

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

  rtoken gen;
  vector<source, 8> read;

  tokenizer*   base = nullptr;
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
  std::int32_t                  pos = 0;
  resolve_state                  rs  = resolve_state::rsset;
};

void transform::do_substitutions(param_substitution const& subs, rtoken_cache const& input,
  rtoken_cache& output)
{
  for (auto const& rt : input)
  {
    if (rt.replace > 0)
    {
      auto const& sub = subs[rt.replace];
      for (auto const& s : sub)
        output.emplace_back(from(s));
    }
    else
      output.push_back(rt);
  }
}

bool transform::is_defined(token_stream& tk)
{
  bool  unexpected = false;
  auto  tok        = tk.get();
  auto  test       = tok;
  if (tok.type == token_type::ty_keyword_ident)
  {
  }
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
    return false;
  }
  else
  {
    return is_defined(value(test));
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

void transform::resolve_tokens(token_stream& tk) 
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
      }
      else
        post(token(rtok));
      rs = resolve_state::rsset;
    }
    token start = tk.get();
    switch (start.type)
    {
    case token_type::ty_blk_comment:
      [[fallthrough]];
    case token_type::ty_sl_comment:
      post(start);
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
        break;
      default:
        post(token(rr));
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
        post(is_defined(tk));
      }
      else
      {
        resolve_identifier(start, sv, tk);
      }
    }
    break;
    default:
      post(start);
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
  while (!err_bit && !done)
  {
    auto          tok   = tk.get();
    std::uint32_t depth = 1;

    switch (type(tok))
    {
    case token_type::ty_eof:
    {
      tf.push_error("unexpected during macro call", tok);
      return;
    }
    break;
    case token_type::ty_operator:
    {
      if (hasop(tok, ','))
      {
        substitutions.emplace_back(std::move(local_cache));
        break;
      }
    }
      [[fallthrough]];
    case token_type::ty_bracket:
    {
      if (hasop(tok, '('))
      {
        if (!depth++)
          break;
      }
      else if (hasop(tok, ')'))
      {
        if (!--depth)
        {
          substitutions.emplace_back(std::move(local_cache));
          done = true;
          break;
        }
      }
    }
      [[fallthrough]];
    default:
      // if next or previous was/is token paste
      local_cache.push_back(tok);
    }
  }

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
  /*

template <typename generator>
void expand_macro_call(transform& tf, transform::macromap::iterator it, generator& tk, transform::token_cache& tcache,
                       transform::generated_tokens& gcache);


bool transform::resolve_tokens(token const& tok, tokenizer& tk, transform::token_cache& tcache,
                               transform::generated_tokens& gcache)
{
  switch (tok.type)
  {
  case token_type::ty_blk_comment:
    [[fallthrough]];
  case token_type::ty_sl_comment:
    break;
  case token_type::ty_newline:
  case token_type::ty_eof:
    return false;
  case token_type::ty_keyword_ident:
  {
    auto v = value(tok);
    if (v == "defined")
    {
      // asking if this is defined
      tcache.push_back(is_defined(tk));
    }
    else
    {
      auto it = macros.find(v);
      if (it != macros.end())
      {
        if (it->second.is_function)
        {
          expand_macro_call(*this, it, tk, tcache, gcache);
        }
        else
        {
          auto const& content = it->second.content;
          resolve_macro_def(content, tcache, gcache);
        }
      }
      else
        tcache.push_back(tok);
    }
  }
  break;
  default:
    tcache.push_back(tok);
  }
  return true;
}

bool transform::resolve_tokens(rtoken const& tok, rtoken_generator& tk, 
                               transform::token_cache& tcache, transform::generated_tokens& gcache)
{
  if (tk.rs == resolve_state::rsjoin)
  {
    if (tcache.empty())
    {
      push_error("incorrect state", "", {});
      return false;
    }
    auto& last = tcache.back();
    if (last.type != token_type::ty_rtoken)
    {
      push_error("incorrect token type", "", {});
      return false;
    }
    auto& rtok = gcache.front();
    assert(&rtok == last.value.rt);
    token_paste(rtok, tok);
    tk.rs = resolve_state::rresolve;
    return !err_bit;
  }
  else if (tok.type == token_type::ty_operator2 && tok.op2_type() == operator2_type::op_tokpaste)
  {
    tk.rs = resolve_state::rsjoin;
    return true;
  }
  else
  {
    auto next = tk.peek();

    if (next.type == token_type::ty_operator2 && next.op2_type() == operator2_type::op_tokpaste)
    {
      gcache.emplace_front(tok);
      auto const& rtok = gcache.front();
      tcache.push_back(rtok);
      return true;
    }
  }
  if (tk.rs == resolve_state::rresolve)
  {
    auto& rtok = gcache.front();
    if (rtok.type == token_type::ty_keyword_ident)
    {
      resolve_macro_def(tok, tk, tcache, gcache);
    }
    tk.rs = resolve_state::rsset;
  }
  switch (tok.type)
  {
  case token_type::ty_blk_comment:
    [[fallthrough]];
  case token_type::ty_sl_comment:
    break;
  case token_type::ty_newline:
  case token_type::ty_eof:
    return false;
  case token_type::ty_keyword_ident:
  {
    if (!resolve_macro_def(tok, tk, tcache, gcache))
      tcache.push_back(tok);
  }
  break;
  default:
    tcache.push_back(tok);
  }
  return true;
}

bool transform::resolve_macro_def(const ppr::rtoken& tok, ppr::transform::rtoken_generator& tk,
                                  ppr::transform::token_cache& tcache, ppr::transform::generated_tokens& gcache)
{
  auto v = value(tok);

  auto it = macros.find(v);
  if (it != macros.end())
  {
    if (it->second.is_function)
    {
      expand_macro_call(*this, it, tk, tcache, gcache);
    }
    else
    {
      auto const& content = it->second.content;
      resolve_macro_def(content, tcache, gcache);
    }
  }
  else
    return false;
  return true;
}

template <typename generator>
void expand_macro_call(transform& tf, transform::macromap::iterator it, generator& tk, transform::token_cache& tcache,
                       transform::generated_tokens& gcache)
{
  auto tok = tk.get();
  while (tok.type == token_type::ty_newline)
  {
    tok = tk.get();
  }
  
  if (tok.type != token_type::ty_bracket || tok.op_type() != '(')
  {
    tf.push_error("unexpected during macro call", tok);
    return;
  }

  auto const&        mdef = it->second;
  transform::token_cache local_cache;
  transform::param_substitution substitutions;
  bool               done = false;
  while (!err_bit && !done)
  {
    auto          tok   = tk.get();
    std::uint32_t depth = 1;

    switch (tok.type)
    {
    case token_type::ty_eof:
    {
      tf.push_error("unexpected during macro call", tok);
      return;
    }
    break;
    case token_type::ty_operator:
    {
      if (tok.op_type() == ',')
      {
        substitutions.emplace_back(std::move(local_cache));
        break;
      }
    }
      [[fallthrough]];
    case token_type::ty_bracket:
    {
      if (tok.op_type() == '(')
      {
        if (!depth++)
          break;
      }
      else if (tok.op_type() == ')')
      {
        if (!--depth)
        {
          substitutions.emplace_back(std::move(local_cache));
          done = true;
          break;
        }
      }
    }
      [[fallthrough]];
    default:
      // if next or previous was/is token paste
      tf.resolve_tokens(tok, tk, local_cache, gcache);
    }
    last = tok;
  }

  if (substitutions.size() != mdef.params.size())
  {
    push_error("mismatch parameter count", tok);
    return;
  }

  tf.resolve_macro_def(mdef.content, substitutions, tcache, gcache);
}

void transform::resolve_macro_def(rtoken_cache const& cc, param_substitution const& subs, token_cache& tcache,
                                  generated_tokens& gcache)
{
  rtoken_generator gen(cc);
  auto             tok = gen.get();
  while (!err_bit && (tok.type != token_type::ty_eof || tok.type != token_type::ty_newline))
  {
    if (tok.replace >= 0)
    {
      subs[]
    }
    tok = gen.get();
  }
    
}

void transform::resolve_macro_def(rtoken_cache const& cc, token_cache& tcache, generated_tokens& gcache) 
{
  rtoken_generator gen(cc);
  auto             tok = gen.get();
  while (!err_bit && (tok.type != token_type::ty_eof || tok.type != token_type::ty_newline) &&
         resolve_macro_def(tok, gen, tcache, gcache))
    tok = gen.get();
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

void transform::token_paste(rtoken & rt, rtoken const& t)
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

      auto v  = value(t);
      auto it = std::find(m.params.begin(), m.params.end(), v);
      if (it != m.params.end())
      {
        m.content.emplace_back(token_type::ty_keyword_ident, v, t.value.td.whitespaces,
                         static_cast<int>(std::distance(m.params.begin(), it)));
      }
      else
        m.content.emplace_back(token_type::ty_keyword_ident, v, t.value.td.whitespaces);
    }
    break;
    case token_type::ty_operator:
    {
      m.content.emplace_back(t.value.td.op, t.value.td.whitespaces);
    }
    break;
    case token_type::ty_operator2:
    {
      m.content.emplace_back(t.value.td.op2, t.value.td.whitespaces);
    }
    break;
    default:
    {
      auto v = value(t);
      m.content.emplace_back(t.type, v, t.value.td.whitespaces, -1);
    }
    break;
    }    
    if (!transform_code && !err_bit)
      post(t);
    t = tk.get();
  }
  if (!transform_code && !err_bit)
    post(t);
}

rtoken transform::from(token const& t) 
{
  switch (t.type)
  {
  case token_type::ty_operator:

    return rtoken(t.value.td.op, t.value.td.whitespaces);

  case token_type::ty_operator2:

    return rtoken(t.value.td.op2, t.value.td.whitespaces);

  default:
  {
    auto v = value(t);
    return rtoken(t.type, v, t.value.td.whitespaces, -1);
  }
  }
}

void transform::read_macro_def(token t, tokenizer& tk, macro& m)
{
  bool tp = false;
  while (!err_bit && t.type != token_type::ty_newline)
  {    
    if (!tp)
    {
      rtoken r = from(t);
      if (r.type == token_type::ty_operator2 && r.op2 == operator2_type::op_tokpaste)
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
    t = tk.get();
  }
  if (!transform_code && !err_bit)
    post(t);
}

std::string transform::read_define(tokenizer& tk, macro& m)
{
  std::string  name;
  auto         get_tok   = [&tk, this](bool print)
  {
    auto t      = tk.get();
    if (print)
      post(t);
    return t;
  };

  token tok;
  tok           = get_tok(!transform_code);

  if (tok.type != token_type::ty_keyword_ident)
  {
    push_error("expecting a macro name", tok);
    return;
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
      auto tok      = get_tok(!transform_code);
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
          return;
        }
        break;
      case token_type::ty_keyword_ident:
        m.params.emplace_back(value(tok));
        break;
      default:
        push_error("unexpected token", tok);
        return;
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


  /*
  auto capture_content = [&](macro::rtoken w) -> token
  {
    bool first = true;
    auto  pair  = lengths[w.replace];
    
    for (std::int16_t i = pair.first, end = pair.second - 1; i < end; ++i)
    {
      auto t = captures[i];
      if (first)
      {
        t.whitespaces = w.t.whitespaces;
        first         = false;
      }
      tcache.push_back(t);
    }
    return pair.second > pair.first ? captures[pair.second - 1] : token{};
  };

  for (std::uint32_t i = 0, en = static_cast<std::uint32_t>(mdef.content.size()); i < en; ++i)
  {
    auto const& w = mdef.content[i];
    // check for token paste operator
    if (i < en - 2 && is_token_paste(mdef.content[i + 1].t))
    {
      // token paste was not resolved earlier
      // x ## y, 3 tokens to be merged, with resolution
      auto x = w;
      auto y = mdef.content[i + 2];

      if (x.replace >= 0)
        x.t = capture_content(x);
                
      if (y.replace >= 0)
        y.t = capture_content(y);

      bool replaced = false;
      if (x.t.type == token_type::ty_keyword_ident)
      {
        // string replace is supported here
        // no macro call
        std::string lookup = std::string{value(x.t)};
        lookup += value(y.t);

        auto it = macros.find(lookup);
        if (it != macros.end())
        {
          if (!it->second.is_function)
          {
            tcache.reserve(it->second.content.size() + tcache.size());
            for (auto& c : it->second.content)
              tcache.push_back(c.t);
            replaced = true;
          }
        }
      }

      if (!replaced)
      {
        tcache.push_back(x.t);
        y.t.whitespaces = 0;
        tcache.push_back(y.t);  
      }

      i += 2;
    }
    else if (w.replace >= 0)
    {
      tcache.push_back(capture_content(w));
    }
    else
      tcache.push_back(w.t);
  }*/

void transform::undefine(tokenizer& tk)
{
  token        tok = tk.get();

  if (tok.type != token_type::ty_keyword_ident)
  {
    push_error("expecting a macro name", tok);
    return;
  }
  else
  {
    auto name = value(tok);
    auto it   = macros.find(name);
    if (it != macros.end())
      macros.erase(it);
  }
}

void transform::push_source(std::string_view source)
{
  tokenizer tk(source, ss);
  token_stream ts(tk);

  content = source;

  token saved;
  while (!err_bit)
  {
    auto tok      = tk.get();
    bool handled  = false;
    bool flip     = false;
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
          macro            m;
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
          section_disabled = is_defined(ts);
          if (flip)
            section_disabled = !section_disabled;
          handled          = true;
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
          section_disabled = !eval(tk);
          handled          = true;
        }
        else
        {
          disable_depth++;
        }
        break;
      case preprocessor_type::pp_elif:
        if (section_disabled)
        {
          if (!disable_depth)
          {
            section_disabled = !eval(tk);
            handled          = true;
          }
        }
        else
          section_disabled = true;
        break;
      case preprocessor_type::pp_else:
        if (section_disabled)
        {
          if (!disable_depth)
          {
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
            section_disabled = false; // unlock
            handled          = true;
          }
          else
            disable_depth--;
        }
        else
          handled = true;
        break;
      case preprocessor_type::pp_undef:
        if (!section_disabled)
          undefine(tk);
        break;
      default:
        if (!handled && (!section_disabled || !ignore_disabled))
          // #
          post(saved);
        break;
      }

      if (!handled && (!section_disabled || !ignore_disabled))
        post(tok);
    }
    break;
    default:
      if (!(tok.type == token_type::ty_operator && tok.value.td.op == '#' && tk.peek().type == token_type::ty_preprocessor))
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

bool transform::eval(tokenizer& tk)
{
  generated_tokens gcache;
  bool done = false;
  while (!err_bit && !done)
  {
    auto tok = tk.get();
    if (!resolve_tokens(tok, tk, cache, gcache))
      done = true;
  }

  // parse
  live_eval le(*this, cache);
  bool      result = eval(le);
  cache.clear();
  return result;
}
bool transform::eval(std::string_view sv)
{
  tokenizer tk(sv, ss);
  content = sv;
  bool result = eval(tk);
  content     = {};
  return result;
}

void transform::push_error(std::string_view s, token const& t)
{
  ss.error(s, value(t), t, t.type != token_type::ty_rtoken ? t.value.td.pos : loc{});
  err_bit = true;
}

void transform::push_error(std::string_view s, std::string_view t, loc const& l)
{
  ss.error(s, t, {}, l);
  err_bit = true;
}

} // namespace ppr