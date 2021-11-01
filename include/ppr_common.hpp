
#pragma once

#include <string>
#include <string_view>
#include <cassert>
#include <vector>
#include <functional>

#ifdef PPR_DYN_LIB_
#if defined _WIN32 || defined __CYGWIN__
#ifdef PPR_EXPORT_
#ifdef __GNUC__
#define PPR_API __attribute__((dllexport))
#else
#define PPR_API __declspec(dllexport)
#endif
#else
#ifdef __GNUC__
#define PPR_API __attribute__((dllimport))
#else
#define PPR_API __declspec(dllimport)
#endif
#endif
#else
#if __GNUC__ >= 4
#define PPR_API __attribute__((visibility("default")))
#elif defined(__clang__)
#define PPR_API __attribute__((visibility("default")))
#else
#define PPR_API
#endif
#endif
#else
#define PPR_API
#endif

#ifdef PPR_SMALL_VECTOR
template <typename T, unsigned N>
using vector = PPR_SMALL_VECTOR<T, N>;
#else
template <typename T, unsigned N>
using vector = std::vector<T>;
#endif

namespace ppr
{
  struct str_equal_test : public std::equal_to<>
  {
    using is_transparent = void;
  };

  struct str_hash
  {
    using is_transparent = void;
    using key_equal = std::equal_to<>;  // Pred to use
    using hash_type = std::hash<std::string_view>;  // just a helper local type
    size_t operator()(std::string_view txt) const { return hash_type{}(txt); }
    size_t operator()(std::string const& txt) const { return hash_type{}(txt); }
    size_t operator()(char const* txt) const { return hash_type{}(txt); }
  };

}