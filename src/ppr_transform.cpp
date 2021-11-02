
#include <ppr_sink.hpp>
#include <ppr_transform.hpp>

namespace ppr
{

bool transform::recursive_resolve_internal(token const& tok, tokenizer& tk, token_cache& tcache)
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
          expand_macro_call(it, tk, tcache);
        }
        else
        {
          auto const& content = it->second.content;
          for (auto const& c : content)
            tcache.push_back(c.t);
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

void transform::read_define(tokenizer& tk, std::string_view& name, macro& m)
{
  std::int16_t source_id = static_cast<std::int16_t>(sources.size() - 1);
  auto         get_tok   = [source_id, &tk, this](bool print)
  {
    auto t      = tk.get();
    t.source_id = source_id;
    if (print)
      post(t);
    return t;
  };

  token tok;
  tok           = get_tok(!transform_code);
  tok.source_id = source_id;

  if (tok.type != token_type::ty_keyword_ident)
  {
    push_error("expecting a macro name", tok);
    return;
  }
  else
  {
    name = value(tok);
    tok  = get_tok(false);
  }

  if (tok.type == token_type::ty_bracket && tok.op == '(' && tok.whitespaces == 0)
  {
    if (!transform_code)
      post(tok);

    m.is_function = true;
    bool done     = false;
    while (!err_bit && !done)
    {
      auto tok      = get_tok(!transform_code);
      tok.source_id = source_id;
      switch (tok.type)
      {
      case token_type::ty_bracket:
        if (tok.op == ')')
        {
          done = true;
        }
        break;
      case token_type::ty_operator:
        if (tok.op == ',')
        {}
        else
        {
          push_error("unexpected operator", tok);
          return;
        }
        break;
      case token_type::ty_keyword_ident:
        m.params.push_back(tok);
        break;
      default:
        push_error("unexpected token", tok);
        return;
      }
    }
    if (!err_bit)
      tok = get_tok(false);
  }

  while (!err_bit && tok.type != token_type::ty_newline)
  {
    if (!recursive_resolve_internal(tok, tk, cache))
      break;
    tok = get_tok(false);
  }

  m.content.reserve(cache.size());
  for (auto const& t : cache)
  {
    bool isparam = false;
    if (m.is_function)
    {
      for (std::uint32_t i = 0, en = static_cast<std::uint32_t>(m.params.size()); i < en; ++i)
      {
        auto const& vt = m.params[i];
        if (vt.length == t.length && value(vt) == value(t))
        {
          macro::rtoken w;

          w.replace = static_cast<int>(i);
          w.t       = t;
          m.content.emplace_back(w);
          isparam = true;
          break;
        }
      }
    }
    if (!isparam)
    {
      macro::rtoken w;
      w.t = t;
      m.content.emplace_back(w);
    }
    if (!transform_code && !err_bit)
      post(t);
  }
  cache.clear();
  if (!transform_code && !err_bit)
    post(tok);
}

void transform::expand_macro_call(macromap::iterator it, tokenizer& tk, token_cache& tcache)
{

  std::int16_t source_id = static_cast<std::int16_t>(sources.size() - 1);
  token        tok;
  do
  {
    tok = tk.get();
  }
  while (tok.type == token_type::ty_newline);

  if (tok.type != token_type::ty_bracket || tok.op != '(')
  {
    push_error("unexpected during macro call", tok);
    return;
  }

  auto const&                                      mdef = it->second;
  vector<std::pair<std::int16_t, std::int16_t>, 4> lengths;
  vector<token, 4>                                 captures;
  token                                            last;
  std::int16_t                                     offset = 0;
  bool                                             done   = false;
  while (!err_bit && !done)
  {
    auto tok            = tk.get();
    tok.source_id       = source_id;
    std::uint32_t depth = 1;

    switch (tok.type)
    {
    case token_type::ty_eof:
    {
      push_error("unexpected during macro call", tok);
      return;
    }
    break;
    case token_type::ty_operator:
    {
      if (tok.op == ',')
      {
        lengths.emplace_back(offset, static_cast<std::int16_t>(captures.size()));
        offset = static_cast<std::int16_t>(captures.size());
        break;
      }
    }
      [[fallthrough]];
    case token_type::ty_bracket:
    {
      if (tok.op == '(')
      {
        if (!depth++)
          break;
      }
      else if (tok.op == ')')
      {
        if (!--depth)
        {
          lengths.emplace_back(offset, static_cast<std::int16_t>(captures.size()));
          done = true;
          break;
        }
      }
    }
      [[fallthrough]];
    default:
      // if next or previous was/is token paste
      if (is_token_paste(tk.peek()) || is_token_paste(last))
        captures.push_back(tok);
      else
        recursive_resolve_internal(tok, tk, captures);
    }
    last = tok;
  }

  if (lengths.size() != mdef.params.size())
  {
    push_error("mismatch parameter count", tok);
    return;
  }

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
  }
}

void transform::undefine(tokenizer& tk)
{
  std::int16_t source_id = static_cast<std::int16_t>(sources.size() - 1);
  token        tok;
  tok           = tk.get();
  tok.source_id = source_id;

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

  sources.push_back(source);

  std::int16_t source_id = static_cast<std::int16_t>(sources.size() - 1);
  assert(source_id < std::numeric_limits<std::int16_t>::max());

  token saved;
  while (!err_bit)
  {
    auto tok      = tk.get();
    tok.source_id = source_id;
    bool handled  = false;
    bool flip     = false;
    switch (tok.type)
    {
    case token_type::ty_eof:
      return;
    case token_type::ty_preprocessor:
    {
      switch (tok.pp_type)
      {
      case preprocessor_type::pp_define:
        if (!section_disabled)
        {
          std::string_view name;
          macro            m;
          if (!transform_code)
          {
            post(saved);
            post(tok);
          }
          read_define(tk, name, m);
          if (!err_bit)
            macros.emplace(name, std::move(m));
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
          section_disabled = flip ? is_defined(tk)  : is_not_defined(tk);
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
      if (!(tok.type == token_type::ty_operator && tok.op == '#' && tk.peek().type == token_type::ty_preprocessor))
      {
        if (transform_code && !section_disabled)
        {
          if (tok.type == token_type::ty_keyword_ident)
          {
            recursive_resolve_internal(tok, tk, cache);
            for (auto const& t : cache)
              post(t);
            cache.clear();
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
}

bool transform::eval(tokenizer& tk)
{
  bool done = false;
  while (!err_bit && !done)
  {
    auto tok = tk.get();
    if (!recursive_resolve_internal(tok, tk, cache))
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
  sources.push_back(sv);
  bool result = eval(tk);
  sources.pop_back();
  return result;
}

void transform::push_error(std::string_view s, token const& t)
{
  ss.error(s, value(t), t, t.pos);
  err_bit = true;
}

void transform::push_error(std::string_view s, std::string_view t, loc const& l)
{
  ss.error(s, t, {}, l);
  err_bit = true;
}

} // namespace ppr