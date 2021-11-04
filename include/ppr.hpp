
// #define PPR_SMALL_VECTOR boost::small_vector // to use stack allocations

#include "ppr_common.hpp"
#include "ppr_eval_type.hpp"
#include "ppr_loc.hpp"
#include "ppr_token.hpp"
#include "ppr_sink.hpp"
#include "ppr_tokenizer.hpp"
#include "ppr_transform.hpp"

#ifdef PPR_IMPL
#include "detail/ppr_sink.hxx"
#include "detail/ppr_tokenizer.hxx"
#include "detail/ppr_transform.hxx"
#include "detail/ppr_tokenizer.cxx"
#ifdef yylex
#undef yylex
#endif
#ifdef YY_DECL
#undef YY_DECL
#endif
#include "detail/ppr_eval.cxx"
#endif

