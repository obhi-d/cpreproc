
#pragma once

#include <cstdint>
#include <string>
#include <ostream>

namespace ppr
{

  struct loc
  {
    std::int32_t line = 0;
    std::int32_t column = 0;
  };

  struct span
  {
    loc begin = {};
    loc end   = {};

    span() = default;
    span(loc l) : begin(l), end(l)
    {
    }
    span(loc l, std::int32_t len) : begin(l), end(l)
    {
      end.column += len;
    }

    inline operator std::string() const
    {
      std::string value;
      value += std::to_string(begin.line);
      value += ":";
      value += std::to_string(begin.column);
      return value;
    }

    friend std::ostream& operator<<(std::ostream& yyo, span const& l)
    {
      yyo << "<" << l.begin.line << '-' << l.begin.column << ">";
      return yyo;
    }
  };
  }