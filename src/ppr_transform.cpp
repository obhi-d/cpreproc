
#include <ppr_transform.hpp>

namespace ppr
{

//template <typename visitor>
bool transform::recursive_resolve_internal(token const& tok, tokenizer& tk, visitor fv)
{
  switch (tok.type)
  {
  case token_type::ty_newline:
  case token_type::ty_eof:
    return false;
  case token_type::ty_keyword_ident:
  {
    auto v  = value(tok);
    if (v == "defined")
    {
      // asking if this is defined
      fv(is_defined(tk));
    }
    else
    {
      auto it = macros.find(v);
      if (it != macros.end())
      {
        if (it->second.is_function)
        {
          expand_macro_call(it, tk, std::forward<visitor>(fv));
        }
        else
        {
          auto const& content = it->second.content;
          for (auto const& c : content)
            fv(c.t);
        }
      }
      else
        fv(tok);
    }
  }
  break;
  case token_type::ty_operator:
    if (tok.op[0] == '!')
    {
      auto v = value(tk.peek());
      if (v == "defined")
      {
        // asking if this is defined
        tk.get();
        auto res = is_defined(tk);
        res.type = res.type == token_type::ty_true ? token_type::ty_false : token_type::ty_true;
        fv(res);
        break;
      }
    }
  default:
    fv(tok);
  }
  return true;
}

void transform::read_define(tokenizer& tk, std::string_view& name, macro& m, listener& l)
{
  auto get_tok = [&](bool print)
  {
    auto t = tk.get();
    if (print)
      post(t, l);
    return t;
  };

  std::int16_t source_id = static_cast<std::int16_t>(sources.size() - 1);
  token tok;
  tok           = get_tok(!transform_code);
  tok.source_id = source_id;

  if (tok.type != token_type::ty_keyword_ident)
  {
    push_error("expecting a macro name", tok, tk.get_loc());
    return;
  }
  else
  {
    name = value(tok);
    tok  = get_tok(false);
  }

  if (tok.type == token_type::ty_bracket && tok.op[0] == '(' && tok.whitespaces == 0)
  {
    if (!transform_code)
      post(tok, l);

    m.is_function = true;
    bool done     = false;
    while (!err_bit && !done)
    {
      auto tok      = get_tok(!transform_code);
      tok.source_id = source_id;
      switch (tok.type)
      {
      case token_type::ty_bracket:
        if (tok.op[0] == ')')
        {
          done = true;
        }
        break;
      case token_type::ty_operator:
        if (tok.op[0] == ',')
        {
        }
        else
        {
          push_error("unexpected operator", tok, tk.get_loc());
          return;
        }
        break;
      case token_type::ty_keyword_ident:
        m.params.push_back(tok);
        break;
      default:
        push_error("unexpected token", tok, tk.get_loc());
        return;
      }
    }
    if (!err_bit)
      tok = get_tok(false);
  }
        
  while (!err_bit && tok.type != token_type::ty_newline)
  {
    tok.source_id = source_id;
    bool isparam  = false;
    if (tok.type == token_type::ty_keyword_ident && m.is_function)
    {
      // is it a param?
      
      for (std::uint32_t i = 0, en = static_cast<std::uint32_t>(m.params.size()); i < en; ++i)
      {
        auto const& vt = m.params[i];
        if (vt.length == tok.length && value(vt) == value(tok))
        {
          macro::wrap w;
          
          w.replace = static_cast<int>(i);
          w.t       = tok;
          m.content.emplace_back(w);
          isparam = true; 
          break;
        }
      }
    }
    
    if (!isparam)
    {
      if (!recursive_resolve_internal(tok, tk, [&](token const& t) {
                                        macro::wrap w;
                                        w.t = t;
                                        if (!transform_code)
                                          post(t, l);
                                        m.content.emplace_back(w);
        }))
        return;
    }
    else if (!transform_code)
      post(tok, l);
    tok = get_tok(false);
  }
  if (!transform_code && !err_bit)
    post(tok, l);
}

//template <typename visitor>
void transform::expand_macro_call(macromap::iterator it, tokenizer& tk, visitor v)
{

  std::int16_t source_id = static_cast<std::int16_t>(sources.size() - 1);
  token tok;
  do
  {
    tok = tk.get();
  }
  while (tok.type == token_type::ty_newline);
  
  if (tok.type != token_type::ty_bracket || tok.op[0] != '(')
  {
    push_error("unexpected during macro call", tok, tk.get_loc());
    return;
  }

  auto const&                                      mdef = it->second;
  vector<std::pair<std::int16_t, std::int16_t>, 4> lengths;
  vector<token, 4>                                 captures;
  std::int16_t                                     length = 0;
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
      push_error("unexpected during macro call", tok, tk.get_loc());
      return;
    }
    case token_type::ty_operator:
    {
      if (tok.op[0] == ',')
      {
        lengths.emplace_back(offset, length);
        offset = length;
        break;
      }
    }
    case token_type::ty_bracket:
    {
      if (tok.op[0] == '(')
      {
        if(!depth++)
          break;
      }
      else if (tok.op[0] == ')')
      {
        if (!--depth)
        {
          lengths.emplace_back(offset, length);
          done = true;
          break;
        }
      }
    }
    default:
      recursive_resolve_internal(tok, tk,
                                 [&](token const& t)
                                 {
                                   captures.push_back(tok);
                                   length++;
                                 });
            
    }
  }

  if (lengths.size() != mdef.params.size())
  {
    push_error("mismatch parameter count", tok, tk.get_loc());
    return;
  }

  for (auto const& w : mdef.content)
  {
    if (w.replace >= 0)
    {
      bool first = true;
      for (std::int16_t i = lengths[w.replace].first, end = lengths[w.replace].second; i < end; ++i)
      {
        token t = captures[i];
        if (first)
        {
          t.whitespaces = w.t.whitespaces;
          first         = false;
        }
        v(t);
      }
    }
    else
      v(w.t);
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
    push_error("expecting a macro name", tok, tk.get_loc());
    return;
  }
  else
  {
    auto name = value(tok);
    auto it = macros.find(name);
    if (it != macros.end())
      macros.erase(it);
  }
}

void transform::push_source(std::string_view source, listener l)
{
  tokenizer tk(source, reporter);
  
  sources.push_back(source);

  std::int16_t source_id = static_cast<std::int16_t>(sources.size() - 1);
  assert(source_id < std::numeric_limits<std::int16_t>::max());
  
  token saved;
  while (!err_bit)
  {
    auto tok = tk.get();
    tok.source_id = source_id;
    bool handled  = false;

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
            post(saved, l);
            post(tok, l);
          }
          read_define(tk, name, m, l);
          if (!err_bit)
            macros.emplace(name, std::move(m));
          handled = true;
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
        undefine(tk);
        break;
      default:
        break;
      }

      if (!handled  && (!section_disabled || !ignore_disabled))
        post(tok, l);
    }
    break;
    default:
      if (!(tok.type == token_type::ty_operator && tok.op[0] == '#' && tk.peek().type == token_type::ty_preprocessor))
      {
        if (transform_code && !section_disabled)
        {
          recursive_resolve_internal(tok, tk,
                                     [&](token const& t)
                                     {
                                       post(t, l);
                                     });
        }
        else if (!section_disabled || !ignore_disabled)
          post(tok, l);
      }
      else
        saved = tok;
    }
  }    
}

bool transform::eval(tokenizer& tk)
{
  cache.clear();
  bool done = false;
  while (!err_bit && !done)
  {
    auto tok = tk.get();
    if (!recursive_resolve_internal(tok, tk, [&](auto const& t) {
      cache.push_back(t);
                                    }))
      done = true;
  }

  // parse
  live_eval le(*this, cache);
  return eval(le);
}
bool transform::eval(std::string_view ss) 
{
  tokenizer tk(ss, reporter);
  sources.push_back(ss);
  bool result = eval(tk);
  sources.pop_back();
  return result;
}
}