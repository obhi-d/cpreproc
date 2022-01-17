
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>

#define PPR_IMPLEMENT
#include <ppr.hpp>

class sink_adapter : public ppr::sink
{
  std::ostream& out;

public:
  sink_adapter(std::ostream& sout) : out(sout) {}
  void handle(ppr::token const& t, symvalue const& data) override
  {
    if (t.was_disabled ^ last_disabled)
    {
      if (t.was_disabled)
        out << "/* ";
      else
        out << "*/ ";
      last_disabled = t.was_disabled;
    }

    out << data.first << data.second;
  }

  void error(std::string_view s, std::string_view e, ppr::token t, ppr::loc l) override
  {
    out << "error : " << s << " - " << e << "l(" << l.line << ":" << l.column << ")" << std::endl;
  }

private:
  bool last_disabled = false;
};


bool compare_expected(std::string const& name)
{
  std::ifstream f1("./output/" + name);
  std::ifstream f2("./references/" + name);

  if (f1.fail() || f2.fail())
  {
    return false; // file problem
  }
  std::string f1_str((std::istreambuf_iterator<char>(f1)), std::istreambuf_iterator<char>());
  std::string f2_str((std::istreambuf_iterator<char>(f2)), std::istreambuf_iterator<char>());

  return f1_str == f2_str;
}

int main(int argc, char* argv[])
{
  int fail     = 0;
  namespace fs = std::filesystem;
  for (auto& p : fs::directory_iterator("./datasets"))
  {
    
    auto path = p.path();
    
    auto        name     = path.filename().generic_string();
    std::string out_file = "./output/";
    out_file += name;

    {
      std::ifstream src(path);
      std::ofstream out(out_file);
      sink_adapter adapter(out);

      sink_adapter sa(out);
      ppr::transform ctx(adapter);
      //ctx.set_transform_code(true);
      if (name.starts_with("d."))
        ctx.set_ignore_disabled(false);
 
      std::stringstream buffer;
      buffer << src.rdbuf();
 
      std::string content = buffer.str();
      ctx.preprocess(content);    
    }

    if (!compare_expected(name))
    {
      std::cout << "failed: " << name << std::endl;
      fail--;
    }
  }
  
  return fail;
}