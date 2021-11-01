
#include <fstream>
#include <iostream>
#include <ppr_transform.hpp>
#include <ppr_sink.hpp>
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

int main(int argc, char* argv[])
{
  sink_adapter   adapter;
  std::string file;
  ppr::transform ctx(adapter);

  for (int i = 1; i < argc; ++i)
  {
    if (std::string(argv[i]) == "-tc")
      ctx.set_transform_code(true);
    else if (std::string(argv[i]) == "-di")
      ctx.set_ignore_disabled(false);
    else if (std::string(argv[i]) == "-dc")
      adapter.set_ignore_comments(false);
    else if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h")
    {
      std::cout << "preprocess [-tc] [-i] [-nc] [--help] file1 file2\n"
                   "  -tc preprocess macro usage in code (experimental)\n"
                   "  -di dont ignore disabled code (print them)\n"
                   "  -dc dont ignore comments (print them)\n";
      std::exit(0);
    }
    else
    {
      file = argv[i];
      std::ifstream     ff(file);
      std::stringstream buffer;
      buffer << ff.rdbuf();
      std::string    content = buffer.str();
      ctx.push_source(content);    
    }
  }
  
  return 0;
}