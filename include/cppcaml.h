
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
    static inline constexpr CppCaml::ApiFunctionEntry fe_v_ ## VARNAME{__VA_ARGS__}; \
    static inline constexpr auto fe_ ## VARNAME \
    __attribute((used, retain, section("caml_api_registry"))) = \
    CppCaml::ApiEntry(&fe_v_ ## VARNAME)

#define CPPCAML_REGISTER_ENUM(VARNAME,...) \
    static inline constexpr CppCaml::ApiFunctionEntry ee_v_ ## VARNAME{__VA_ARGS__}; \
    static inline constexpr auto ee_ ## VARNAME \
    __attribute((used, retain, section("caml_api_registry"))) = \
    CppCaml::ApiEntry(&ee_v_ ## VARNAME)

/////////////////////////////////////////////////////////////////////////////////////////
/// Containers

template<typename T> static inline T& Custom_value(value v){
  return (*((T*)Data_custom_val(v)));
}

template<typename T> void finalize_custom(value v_custom){
  Custom_value<T>(v_custom).~T();
}

template<typename Container> struct ContainerOps {
  static inline struct custom_operations value =
    { typeid(Container).name()
    , &finalize_custom<Container>
    , custom_compare_default
    , custom_hash_default
    , custom_serialize_default
    , custom_deserialize_default
    , custom_compare_ext_default
    , custom_fixed_length_default
    };
};

static const int custom_used = 1;
static const int custom_max = 1000000;

template<typename T>
struct SharedPointerContainer : private boost::noncopyable {
  std::shared_ptr<T> t;

  SharedPointerContainer(std::shared_ptr<T>&& t) : t(std::move(t)) {}

  static inline value allocate(std::shared_ptr<T>&& t){
    typedef SharedPointerContainer<T> This;
    value v_container =
      caml_alloc_custom(&ContainerOps<This>::value,sizeof(This),custom_used, custom_max);
    new(&Custom_value<This>(v_container)) This(std::move(t));
    return v_container;
  };
};


/////////////////////////////////////////////////////////////////////////////////////////
/// Conversions

template<typename T> class AutoConversion;

enum class AutoConversionKind {
  SharedPointer, WithSharedPointerContext
};

namespace AutoConv {
  template <typename T, AutoConversionKind k>
    concept As = 
    AutoConversion<T>::kind == k;

  template <typename T>
    concept HasDeleter = requires() {
      AutoConversion<T>::delete_T;
    };

  template <typename T>
    concept HasContext = requires() {
      typename AutoConversion<T>::Context;
    };
}


template<typename T>
struct CamlConversion{
  static_assert(always_false<T>::value,
      "You must specialize CamlConversion<> for your type");
}; 

template<>
struct CamlConversion<bool> {
  struct ToValue {
    static const bool allocates = false;

    static inline value c(bool& b){
      return Val_bool(b);
    }
  };

  struct OfValue {
    struct Representative {
      bool v;
      bool& get() { return v; };
    };

    static inline Representative c(value v){
      return { .v = (bool)Bool_val(v) };
    }
  };
};

template<typename T>
struct CamlConversionSharedPointer {
  typedef SharedPointerContainer<T> Container;

  struct ToValue {
    static const bool allocates = true;

    static inline value c(std::shared_ptr<T>&& b){
      return Container::allocate(b);
    }
  };

  struct OfValue {
    struct Representative {
      Container&v;
      
      operator T&() { return v.t; };
    };

    static inline Representative c(value v){
      return { .v = Custom_value<Container>(v) };
    }
  };
};

template<> struct CamlConversionSharedPointer<std::string>;

template<typename T_pointer>
requires
(AutoConv::As<T_pointer,AutoConversionKind::SharedPointer>
 && std::is_pointer_v<T_pointer>)
struct CamlConversion<T_pointer> {
  using A = AutoConversion<T_pointer>;
  using T = std::remove_pointer_t<T_pointer>;
  using C = CamlConversionSharedPointer<T>;
  using Container = C::Container;

  struct ToValue : public C::ToValue {
    template<class Dummy = void>
      requires AutoConv::HasDeleter<T_pointer>
    struct Deleter {
      void operator()(T_pointer p){
        A::delete_T(p);
      }
    };

    static inline value c(T_pointer tp){
      if constexpr(AutoConv::HasDeleter<T_pointer>) {
        std::shared_ptr<T> s(tp,Deleter());
        return value(s);
      } else {
        std::shared_ptr<T> s(tp);
        return value(s);
      }
    };
  };

  struct OfValue : public C::OfValue {
    operator T_pointer(){ ((Container&)(*this)).get(); }
  };
};

template<typename T>
requires
(AutoConv::As<T,AutoConversionKind::WithSharedPointerContext>
 && AutoConv::HasContext<T>
)
struct CamlConversion<T> {
  using A = AutoConversion<T>;

  using Context = A::Context;

};

/////////////////////////////////////////////////////////////////////////////////////////
/// Function Properties

template <auto T, typename Property>
  struct FunctionProperty;

template <auto T, typename Property>
concept PropertyDefined = requires {
  FunctionProperty<T,Property>::value;
};

template<auto p_value>
struct P_Define {
  static constexpr const decltype(p_value) value = p_value;
};

struct P_BoolDefaultTrue {
  using type = bool;
  static constexpr const type default_value = true;
};

struct P_BoolDefaultFalse {
  using type = bool;
  static constexpr const type default_value = true;
};

struct P_MayRaiseToOcaml : public P_BoolDefaultTrue {};
struct P_MayReleaseLock : public P_BoolDefaultFalse {};
struct P_ImplicitFirstArgument : public P_BoolDefaultFalse {};

template<auto T, typename Property>
constexpr const Property::type
get_function_property(){
  if constexpr(PropertyDefined<T,Property>){
    return FunctionProperty<T,Property>::value;
  } else {
    return Property::default_value;
  }
};


#define F_PROP(f,prop,value) \
  template<> struct FunctionProperty<f,P_ ## prop> : public P_Define<value> {}

extern "C" void f(){}
F_PROP(f,MayRaiseToOcaml,true);


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
