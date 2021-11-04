
#include <fstream>
#include <iostream>
#include <ppr_transform.hpp>
#include <ppr_sink.hpp>
#include <sstream>
#include <string>

class sink_adapter : public ppr::sink
{
public:
  void handle(ppr::token const& t, symvalue const& data) override
  {
    if (t.was_disabled ^ last_disabled)
    {
      if (t.was_disabled)
        std::cout << "/* ";
      else
        std::cout << "*/ ";
      last_disabled = t.was_disabled;
    }

    std::cout << data.first << data.second;
  }

  void error(std::string_view s, std::string_view e, ppr::token t, ppr::loc l) override
  {
    std::cerr << "error : " << s << " - " << e << "l(" << l.line << ":" << l.column << ")" << std::endl;
  }

private:
  bool last_disabled = false;
};


int main(int argc, char* argv[])
{
  sink_adapter   adapter;
  std::string file;
  ppr::transform ctx(adapter);

  for (int i = 1; i < argc; ++i)
  {
    if (std::string(argv[i]) == "-P")
      ctx.set_transform_code(true);
    else if (std::string(argv[i]) == "-D")
      ctx.set_ignore_disabled(false);
    else if (std::string(argv[i]) == "-K")
      adapter.set_ignore_comments(false);
    else if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-H")
    {
      std::cout << "preprocess [-T] [-I] [-K] [--help, -H] file1 file2\n"
                   "  -P preprocess macro usage in code (experimental)\n"
                   "  -D dont ignore disabled code (print them)\n"
                   "  -K dont ignore comments (print them)\n";
      std::exit(0);
    }
    else
    {
      file = argv[i];
      std::ifstream     ff(file);
      std::stringstream buffer;
      buffer << ff.rdbuf();
      std::string    content = buffer.str();
      ctx.preprocess(content);    
    }
  }
  
  return 0;
}