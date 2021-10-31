
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <functional>

#include <ppr_loc.hpp>
#include <ppr_token.hpp>

namespace ppr
{

class PPR_API context
{
public:
  context(std::string_view ss) : content(ss) {}

  inline token make_op(operator_type type, int len)
  {
    token current;

    current.type        = token_type::ty_operator;
    current.whitespaces = whitespaces;
    current.op          = type;
    current.start       = pos_commit;
    current.length      = len;

    pos_commit += len;
    whitespaces = 0;
    return current;
  }

  inline token make_ppr(preprocessor_type type, int len)
  {
    token current;

    current.type        = token_type::ty_operator;
    current.whitespaces = whitespaces;
    current.pp_type     = type;
    current.start       = pos_commit;
    current.length      = len;

    pos_commit += len;
    whitespaces = 0;
    return current;
  }

  inline token make_token(token_type type, int len)
  {
    token current;

    current.type        = type;
    current.whitespaces = whitespaces;
    current.start       = pos_commit;
    current.length      = len;

    pos_commit += len;
    whitespaces = 0;
    return current;
  }

  inline token make_ident(int len)
  {
    return make_token(token_type::ty_keyword_ident, len);
  }

  inline token make_integer(int len)
  {
    return make_token(token_type::ty_integer, len);
  }

  inline token make_hex_integer(int len)
  {
    return make_token(token_type::ty_hex_integer, len);
  }

  inline token make_real_number(int len)
  {
    return make_token(token_type::ty_real_number, len);
  }

  inline token make_braces(char op)
  {
    auto tok = make_token(token_type::ty_braces, 1);
    tok.op = {op, 0};
    return tok;
  }

  inline token make_bracket(char op)
  {
    auto tok = make_token(token_type::ty_bracket, 1);
    tok.op = {op, 0};
    return tok;
  }

  inline token make_sl_comment(int len)
  {
    return make_token(token_type::ty_sl_comment, len);
  }

  inline token make_blk_comment(int len)
  {
    return make_token(token_type::ty_blk_comment, len);
  }

  inline token make_newline()
  {
    return make_token(token_type::ty_newline, 1);
  }

  inline token make_string(int len) 
  {
    return make_token(token_type::ty_string, len);
  }

  int read(char* data, int size)
  {
    auto min = std::min<std::int32_t>(static_cast<std::int32_t>(content.size() - pos), size);
    if (min)
    {
      std::memcpy(data, content.data() + pos, static_cast<std::size_t>(min));
      pos += min;
      if (min < size)
      {
        data[min]     = EOF;
      }

      assert(min <= size);
      return min;
    }
    data[0] = EOF;
    return 0;
  }

  inline void whitespace(int len)
  {
    whitespaces = len;
    pos_commit += static_cast<std::uint32_t>(len);
  }

  inline int whitespace() const
  {
    return whitespaces;
  }

  inline void start_read_len()
  {
    len_reading = 0;
  }

  inline void read_len(int l)
  {
    len_reading += l;
  }

  inline int flush_read_len()
  {
    int r       = len_reading;
    len_reading = 0;
    return r;
  }

  void push_error(std::string_view error);
  void push_error(std::string_view error, std::string_view token);

  inline void columns(int len)
  {
    location.column += len;
  }

  inline void lines(int len)
  {
    location.line += len;
    location.column = 0;
  }

  loc get_loc() const
  {
    return location;
  }

  void begin_scan();
  void end_scan();

  void print_tokens();

  void end() {}

  void tokenize(std::function<void(ppr::token)>&&);

 private:

  loc                location;
  int                whitespaces = 0;
  
  std::string_view content;
  std::int32_t     pos         = 0;
  std::int32_t     pos_commit  = 0;
  std::int32_t     len_reading = 0;
  std::uint32_t    flags       = 0;

  void*            token_scanner = nullptr;
};

} // namespace ppr