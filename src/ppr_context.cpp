
#include <ppr_context.hpp>
#include <iostream>

extern ppr::token ppr_tokenize(ppr::context& ctx, void* yyscanner);

namespace ppr
{

void context::push_error(std::string_view error) 
{
  std::cerr << "[ERROR] " << error << " (" << location.line << ":" << location.column << ") " <<  std::endl;
}

void context::push_error(std::string_view error, std::string_view what) 
{
  std::cerr << "[ERROR] " << error << ": " << what << " (" << location.line << ":" << location.column << ") "
            << std::endl;
}

void context::tokenize(std::function<void(ppr::token)>&& l)
{
  begin_scan();
  while (true)
  {
    auto tok = ppr_tokenize(*this, token_scanner);
    l(tok);
    if (tok.type == ppr::token_type::ty_eof)
      break;
  } 
    
  end_scan();
}

void context::print_tokens()
{
  tokenize(
    [this](ppr::token t) -> void
    {
      std::cout << std::string(t.whitespaces, ' ') << std::string_view((std::uint32_t)t.start + content.data(), (std::uint32_t)(t.length))
                ;
    });
  std::cout << std::endl;
}

}
