
#include <fstream>
#include <iostream>
#include <ppr_sink.hpp>
#include <ppr_tokenizer.hpp>
#include <sstream>
#include <string>

class sink_adapter : public ppr::sink
{
public:
  void handle(ppr::token const& t, symvalue const& data) override
  {
    std::cout << data.first << data.second;
  }

  void error(std::string_view s, std::string_view e, ppr::token t, ppr::loc l) override
  {
    std::cerr << "error : " << s << " - " << e << "l(" << l.line << ":" << l.column << ")" << std::endl;
  }
};

int main(int argc, char* argv[])
{
  sink_adapter   adapter;
  std::string    file;
  

  for (int i = 1; i < argc; ++i)
  {
    if (std::string(argv[i]) == "--nc")
      adapter.set_ignore_comments(false);
    else
    {
      file = argv[i];
      std::ifstream     ff(file);
      std::stringstream buffer;
      buffer << ff.rdbuf();
      std::string content = buffer.str();
      ppr::tokenizer ctx(content, adapter);
      ctx.print_tokens();
    }
  }
  
  return 0;
}