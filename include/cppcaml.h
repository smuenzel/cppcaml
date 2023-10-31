
#include <caml/memory.h>
#include <caml/mlvalues.h>
#include <caml/alloc.h>
#include <caml/custom.h>
#include <caml/threads.h>
#include <caml/callback.h>
#include <caml/fail.h>
#include <caml/domain_state.h>

#include <boost/core/noncopyable.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <memory>
#include <typeinfo>
#include <functional>
#include <cassert>
#include <cstring>
#include <optional>

#define apireturn extern "C" CAMLprim value

namespace CppCaml {

typedef const char* cstring;

template<typename T>
struct always_false : std::false_type {};

/////////////////////////////////////////////////////////////////////////////////////////
/// Names of Types

template<typename T>
struct ApiTypename{
  static_assert(always_false<T>::value,
      "You must specialize ApiTypename<> for your type");
}; 

template<typename T> struct ApiTypename<const T> : ApiTypename<T> {};
template<typename T> struct ApiTypename<const T*> : ApiTypename<T*> {};
template<typename T> struct ApiTypename<T&> : ApiTypename<T> {};
template<typename T> struct ApiTypename<const T&> : ApiTypename<T> {};

/* https://stackoverflow.com/a/75619411
 */ 
template<size_t ...Len>
constexpr auto cat(const std::array<char,Len>&...strings){
  constexpr size_t N = (... + Len) - sizeof...(Len);
  std::array<char, N + 1> result = {};
  result[N] = '\0';

  auto it = result.begin();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
  (void)((it = std::copy_n(strings.cbegin(), Len-1, it), 0), ...);
#pragma GCC diagnostic pop
  return result;
}

using std::to_array;

template<typename T, size_t L>
constexpr const std::array<T,L>& to_array(const std::array<T,L>& a) { return a; }

template<typename ...Ts>
constexpr auto cat(const Ts&...strings){
  return cat(to_array(strings)...);
}

template<typename ...Ts>
static constexpr auto wrap_paren(const Ts&...strings){
  return cat("(", strings..., ")");
}

template<auto s> struct StaticString {
  static constexpr const auto va = s;
  static constexpr auto value_len = va.size();
  static constexpr const char * value = &va[0];
};

template<typename T> struct ApiTypename<std::vector<T> > {
  typedef StaticString<
    wrap_paren(ApiTypename<T>::name::va," array")
    > name;
};

template<typename T0, typename T1> struct ApiTypename<std::pair<T0,T1> > {
  typedef StaticString<
    wrap_paren(ApiTypename<T0>::name::va, "*", ApiTypename<T1>::name::va)
  > name;
};

template<typename T> struct ApiTypename<std::optional<T> > {
  typedef StaticString<
    wrap_paren(ApiTypename<T>::name::va," option)")
    > name;
};

#define DECL_API_TYPENAME(c_type, caml_type) \
  template<> \
  struct CppCaml::ApiTypename<c_type>{ \
    typedef StaticString<to_array(#caml_type)> name;\
  } 

/////////////////////////////////////////////////////////////////////////////////////////
/// Overload Resolution

template<typename ...T> struct type_list {};

template<class C, typename R, typename... Ps>
constexpr auto resolveOverload(type_list<R,Ps...>, R (C::*fun)(Ps...)){
  return fun;
}

template<class C, typename R, typename... Ps>
constexpr auto resolveOverload(type_list<R,Ps...>, R (C::*fun)(Ps...) const){
  return fun;
}

/////////////////////////////////////////////////////////////////////////////////////////
/// Function/Enum Description

template<typename T>
struct CamlLinkedList{
  const T data;
  const CamlLinkedList<T>*next;

  constexpr CamlLinkedList(T data, const CamlLinkedList<T>*next=nullptr)
    : data{data}, next{next} {}
};

template<typename... Ps> struct ParamList;

template<> struct ParamList<>{
  static constexpr const CamlLinkedList<cstring>* p = nullptr;
};

template<typename P, typename... Ps> struct ParamList<P, Ps...>{
  static inline constexpr const CamlLinkedList<cstring> pp =
    CamlLinkedList(ApiTypename<P>::name,ParamList<Ps...>::p);
  static inline constexpr const CamlLinkedList<cstring>* p = &pp;
};

struct ApiTypeDescription {
  cstring    name;
  const bool conversion_allocates;
};

struct ApiFunctionDescription {
  ApiTypeDescription                       return_type;
  const CamlLinkedList<ApiTypeDescription>*parameters;
  const size_t                             parameter_count;
  const bool may_raise_to_ocaml;
  const bool may_release_lock;          
};

static constexpr const uint64_t marker_value = 0xe1176dafdeadbeefl;

struct ApiFunctionEntry {
  cstring wrapper_name;
  cstring function_name;
  const std::optional<cstring> class_name;
  const ApiFunctionDescription description;

  value to_value();
};

struct ApiEnumEntry {
  cstring enumName;
  cstring memberName;
  const value memberValue;

  value to_value();
};

enum class ApiEntryKind {
  Function, EnumMember
};

struct ApiEntry {
  const uint64_t marker;
  const ApiEntryKind kind;
  union {
    const ApiFunctionEntry*as_function;
    const ApiEnumEntry*as_enum;
  };

  constexpr ApiEntry(const ApiFunctionEntry* f)
    : marker(marker_value), kind(ApiEntryKind::Function), as_function(f)
  {}

  constexpr ApiEntry(const ApiEnumEntry* e)
    : marker(marker_value), kind(ApiEntryKind::EnumMember), as_enum(e)
  {}

  value to_value();
};

/////////////////////////////////////////////////////////////////////////////////////////
/// Registration helpers

#define CPPCAML_REGISTER_FUN(VARNAME,...) \
    static inline constexpr CppCaml::ApiFunctionEntry fe_ ## VARNAME{__VA_ARGS__}; \
    static inline constexpr auto VARNAME \
    __attribute((used, retain, section("caml_api_registry"))) = \
    CppCaml::ApiEntry(&fe_ ## VARNAME)

/////////////////////////////////////////////////////////////////////////////////////////
}
/////////////////////////////////////////////////////////////////////////////////////////
/// Simple types

DECL_API_TYPENAME(bool, bool);

/////////////////////////////////////////////////////////////////////////////////////////
/// Checks

template struct CppCaml::ApiTypename<std::optional<bool> >;
template struct CppCaml::ApiTypename<std::vector<bool> >;
template struct CppCaml::ApiTypename<std::pair<bool,std::vector<bool> >>;

CPPCAML_REGISTER_FUN(example, .wrapper_name = "hello");
