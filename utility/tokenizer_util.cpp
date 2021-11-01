
#include <fstream>
#include <iostream>
#include <ppr_sink.hpp>
#include <ppr_tokenizer.hpp>
#include <sstream>
#include <string>

class sink_adapter : public ppr::sink
{
public:
  void handle(ppr::token const& t, std::string_view data) override
  {
    std::cout << std::string((std::size_t)t.whitespaces, ' ') << data;
  }

  void error(std::string_view s, std::string_view e, ppr::token t, ppr::loc l) override
  {
    std::cerr << "error : " << s << " - " << e << "l(" << l.line << ":" << l.column << ")" << std::endl;
  }
};

int main()
{
  bool quit = false;
  while (!quit)
  {
    std::string file;
    std::cout << "file: ";
    std::cin >> file;
    if (file == "q" || file == "quit")
      quit = true;
    else
    {
      std::ifstream     ff(file);
      std::stringstream buffer;
      buffer << ff.rdbuf();
      std::string    content = buffer.str();
      sink_adapter   adapter;
      ppr::tokenizer ctx(content, adapter);
      ctx.print_tokens();
    }
  }
  return 0;
}