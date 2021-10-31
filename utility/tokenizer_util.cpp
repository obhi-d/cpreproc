
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ppr_context.hpp>

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
      std::ifstream ff(file);
      std::stringstream buffer;
      buffer << ff.rdbuf();
      std::string content = buffer.str();
      ppr::context ctx(content);
      ctx.print_tokens();
    }
  }
  return 0;
}