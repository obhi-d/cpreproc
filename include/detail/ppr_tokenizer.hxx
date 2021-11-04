
#include "ppr_tokenizer.hpp"
#include "ppr_sink.hpp"

extern ppr::token ppr_tokenize(ppr::tokenizer& ctx, void* yyscanner);

namespace ppr
{

void tokenizer::push_error(std::string_view error) 
{
  reporter.error(error, "", {}, location);
}

void tokenizer::push_error(std::string_view error, std::string_view what) 
{
  reporter.error(error, what, {}, location);
}

token tokenizer::get()
{
  if (ahead)
  {
    ahead = false;
    return lookahead;
  }
  return ppr_tokenize(*this, token_scanner);
}

token tokenizer::peek() 
{
  if (!ahead)
    lookahead = ppr_tokenize(*this, token_scanner);
  ahead     = true;
  return lookahead;
}

void  tokenizer::print_tokens()
  {
  for_each(
    [this](ppr::token t) -> void
    {
        std::cout << std::string(t.value.td.whitespaces, ' ')
                  << std::string_view((std::uint32_t)t.value.td.start + content.data(),
                                      (std::uint32_t)(t.value.td.length))
                ;
    });
}

}
