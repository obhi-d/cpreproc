
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

#include <cpreproc_loc.hpp>

namespace cpreproc
{

  class context
  {
  public:

    struct equal_test : public std::equal_to<>
    {
      using is_transparent = void;
    };

    struct string_hash 
    {
      using is_transparent = void;
      using key_equal = std::equal_to<>;  // Pred to use
      using hash_type = std::hash<std::string_view>;  // just a helper local type
      size_t operator()(std::string_view txt) const { return hash_type{}(txt); }
      size_t operator()(std::string const& txt) const { return hash_type{}(txt); }
      size_t operator()(char const* txt) const { return hash_type{}(txt); }
    };

    struct macro_def
    {
      std::vector<std::string> params;
      std::vector<std::string> tokens;
      bool is_function = false;
    };

    using macromap = std::unordered_map<std::string, macro_def, string_hash, equal_test>;

    struct macro_call
    {
      macromap::const_iterator it;
      std::unordered_map<std::string_view, std::string> replacements;
      std::uint32_t paren_scope = 0;
      std::uint32_t param = 0;

      macro_call(macromap::const_iterator i) : it(i) {}
    };
        

    static constexpr std::uint32_t empty_lines = 1;
    static constexpr std::uint32_t delete_comments = 2;
    static constexpr std::uint32_t debug = 2;
    static constexpr std::uint32_t no_replace = 2;

    void begin_scan();
    void end_scan();

    bool ignore_comments() const
    {
      return (flags_ & delete_comments) != 0;
    }

    bool ignore_replacements() const
    {
      return (flags_ & no_replace) != 0;
    }

    void put(char const* value, std::uint32_t len)
    {
      content.append(value, len);
    }

    void put(char value)
    {
      content.append(value);
    }

    void put(std::string& const value)
    {
      content.append(value);
    }

    auto is_macro(std::string_view name) const
    {
      return macros.find(name);
    }

    bool is_macro(macromap::const_iterator it) const
    {
      return ret.it != macros.end();
    }

    bool is_function(macromap::const_iterator it) const
    {
      return ret.it->second.is_function;
    }

    std::string const& replace_with(macromap::const_iterator it) const
    {
      return  it->second.content;
    }

    void next_line(bool append)
    {
      if (append)
        content.append('\n');
      loc.line++;
    }

    void add_columns(std::uint32_t l)
    {
      loc.column += l;
    }

    void end_macro()
    {
      macro_def.content = std::move(content);
      macros.emplace(std::move(current_macro), std::move(macro_def));
    }


    bool pop_eval();

    void set_capture_code_in_scope(bool val)
    {
      condition_evals.back().capture_code_in_scope = val;
    }

    void begin_call(macromap::const_iterator it)
    {
      content_stack.emplace_back(std::move(content));
      macro_calls.emplace_back(it);
    }

    void call_next_param()
    {
      auto const& m = macro_calls.back();
      m.replacements.emplace_back(std::string_view(m.it->second.params[m.param++]), std::move(content));
    }

    void call_push_paren_scope()
    {
      macro_calls.back().paren_scope++;
    }

    std::uint32_t call_pop_paren_scope()
    {
      return --macro_calls.back().paren_scope;
    }

    void end_call()
    {
      auto const& call = macro_calls.back();
      auto const& def = macro_calls.back().it->second;

      if (!def.params.empty())
        call_next_param();

      if (def.params.size() != call.param)
      {
        push_error("parameters count mismatch in call/declaration");
        return;
      }

      content = std::move(content_stack.back());
      content_stack.pop_back();

      auto const& m = call.replacements;

      for (auto const& t : def.tokens)
      {
        auto it = m.find(t);
        if (it != m.end())
        {
          content.append(' ');
          content += it->second;
        }
        else
        {
          content.append(' ');
          content += t;
        }
      }
    }

    void save_source_state(int state)
    {
      saved_state = state;
    }

    int last_source_state() const
    {
      return saved_state;
    }


    void push_error(std::string_view);
  private:

    struct condition
    {
      bool capture_code_in_scope = false;
      bool eval;
    };

    std::string current_macro;
    macro_def   macro_def;

    location loc;
    macromap macros;
    std::vector<std::string> content_stack;
    std::vector<condition> condition_evals;
    std::vector<macro_call> macro_calls;
    std::string content;
    std::uint32_t flags_ = 0;
    int saved_state = 0;
  };
}