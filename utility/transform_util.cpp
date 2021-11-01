
#include <fstream>
#include <iostream>
#include <ppr_transform.hpp>
#include <sstream>
#include <string>

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
      std::string         content = buffer.str();
      ppr::error_reporter lambda  = [](std::string_view s, std::string_view e, ppr::token t, ppr::loc l)
      {
        std::cerr << "error : " << s << " - " << e << "l(" << l.line << ":" << l.column << ")" << std::endl;
      };
      ppr::transform ctx(lambda);
      ctx.push_source(content, [](std::string_view s) {
                        std::cout << s;
        });
    }
  }
  return 0;
}