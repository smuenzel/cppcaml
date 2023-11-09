
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
  static constexpr const type default_value = false;
};

struct P_MayRaiseToOcaml : public P_BoolDefaultTrue {};
struct P_ReleasesLock : public P_BoolDefaultFalse {};
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
  template<> struct CppCaml::FunctionProperty<f,P_ ## prop> : public P_Define<value> {}

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

struct ApiTypeDescription {
  cstring    name;
  // Only set for types that can be converted to ocaml
  const std::optional<bool> conversion_allocates;

  value to_value() const;
};

struct ApiFunctionDescription {
  ApiTypeDescription                       return_type;
  const CamlLinkedList<ApiTypeDescription>*parameters;
  const bool may_raise_to_ocaml;
  const bool releases_lock;          
  const bool has_implicit_first_argument;

  value to_value() const;
};

static constexpr const uint64_t marker_value = 0xe1176dafdeadbeefl;

struct ApiFunctionEntry {
  cstring wrapper_name;
  cstring function_name;
  const std::optional<cstring> class_name;
  const ApiFunctionDescription description;

  value to_value() const;
};

struct ApiEnumEntry {
  cstring enumName;
  cstring memberName;
  const value memberValue;

  value to_value() const;
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

  value to_value() const;
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

    static inline value c(const bool& b){
      return Val_bool(b);
    }
  };

  struct OfValue {
    struct Representative {
      bool v;
      operator bool&() { return v; }
    };

    static inline Representative c(value v){
      return { .v = (bool)Bool_val(v) };
    }
  };
};

struct Void {};

template<>
struct CamlConversion<Void> {
  struct ToValue {
    static const bool allocates = false;

    static inline value c(const Void &){
      return Val_unit;
    }
  };
};

template<>
struct CamlConversion<cstring> {
  struct ToValue {
    static const bool allocates = true;

    static inline value c(const cstring& s){
      if(s)
        return caml_copy_string(s);
      else
        return caml_copy_string("");
    }
  };

  struct OfValue {
    struct Representative {
      cstring v;
      operator cstring&() { return v; }
    };

    static inline Representative c(value v){
      return { .v = String_val(v) };
    }
  };
};

template<typename T>
struct CamlConversion<std::optional<T>> {
  using Inner = CamlConversion<T>;

  struct ToValue {
    static const bool allocates = true;

    static inline value c(const std::optional<T>&t){
      if(t) {
        CAMLparam0();
        CAMLlocal2(inner, outer);
        inner = Inner::ToValue::c(t.value());
        outer = caml_alloc_small(1,0);
        Field(outer,0) = inner;
        CAMLreturn(outer);
      } else {
        return Val_none;
      }
    }
  };

  /* CR smuenzel: not completed */

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
    struct Representative : public C::OfValue::Representative {
      operator T_pointer(){ ((Container&)(*this)).get(); }
    };
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

  struct ToValue {

  };

  struct OfValue {

  };
};

template <typename T>
concept HasContext = requires() {
  typename CamlConversion<T>::Context;
};

/////////////////////////////////////////////////////////////////////////////////////////
/// Creation of Function Descriptions

template<typename T>
static consteval const ApiTypeDescription make_type_description(){
  std::optional<bool> conversion_allocates;
  if constexpr (requires { typename CamlConversion<T>::ToValue; } )
    conversion_allocates = CamlConversion<T>::ToValue::allocates;
  ApiTypeDescription t{
    .name = ApiTypename<T>::name::value,
    .conversion_allocates = conversion_allocates
  };
  return t;
};

template<typename R, typename... Ps>
static consteval const ApiTypeDescription make_return_type(R (*fun)(Ps...)){
  return make_type_description<R>();
}

template<typename... Ps> struct ParamList;

template<> struct ParamList<>{
  static constexpr const CamlLinkedList<ApiTypeDescription>* p = nullptr;
};

template<typename P, typename... Ps> struct ParamList<P, Ps...>{
  static inline constexpr const CamlLinkedList<ApiTypeDescription> pp =
    CamlLinkedList(make_type_description<P>(),ParamList<Ps...>::p);
  static inline constexpr const CamlLinkedList<ApiTypeDescription>* p = &pp;
};

template <typename R, typename... Ps>
static consteval const CamlLinkedList<ApiTypeDescription>*
make_parameters(R (*fun)(Ps...)){
  return ParamList<Ps...>::p;
}

template<auto F>
static consteval const
ApiFunctionDescription
make_function_description(){
  auto return_type = make_return_type(F);
  auto may_raise_to_ocaml =
    get_function_property<F,P_MayRaiseToOcaml>();
  auto releases_lock =
    get_function_property<F,P_ReleasesLock>();
  auto has_implicit_first_argument =
    get_function_property<F,P_ImplicitFirstArgument>();
  return ApiFunctionDescription{
    .return_type = return_type,
    .parameters = make_parameters(F),
    .may_raise_to_ocaml = may_raise_to_ocaml,
    .releases_lock = releases_lock,
    .has_implicit_first_argument = has_implicit_first_argument
  };
}

/////////////////////////////////////////////////////////////////////////////////////////
/// Calling

// Needed so that we can expand the parameter pack. There must be a better way.....
template<typename T_first, typename T_second> using first_type = T_first;
template<typename T_first, size_t T_second> using first_type_s = T_first;

// Invoke a function, and possibly substitute void return arguments

// https://stackoverflow.com/a/51454205
template<typename F, typename... Ps, size_t... is, typename Result = std::invoke_result_t<F,Ps...>>
requires (!std::same_as<Result,void>)
inline Result invoke_seq_void(F&& f, std::tuple<Ps&...>& ps, std::index_sequence<is...> iseq){
  return std::invoke(std::forward<F>(f), get<is>(ps)...);
};

template<typename F, typename... Ps, size_t... is, typename Result = std::invoke_result_t<F,Ps...>>
requires (std::same_as<Result,void>)
inline Void invoke_seq_void(F&& f, std::tuple<Ps&...>& ps, std::index_sequence<is...> iseq){
  std::invoke(std::forward<F>(f), get<is>(ps)...);
  return {};
};

// End Invoke

template<typename... Ts> struct TypeList {
};

template<size_t N, typename T> struct TypeListElt;

template<size_t N, typename T, typename... Ts>
struct TypeListElt<N,TypeList<T,Ts...>>
  : TypeListElt<N-1,TypeList<Ts...>> {};

template<typename T, typename... Ts>
struct TypeListElt<0,TypeList<T, Ts...>> { using type = T; };

template <size_t N, typename T> using TypeListN =
  TypeListElt<N,T>::type;

// https://devblogs.microsoft.com/oldnewthing/20200713-00/?p=103978
template<typename F> struct FunctionTraits;

template<typename R, typename... Args>
struct FunctionTraits<R(*)(Args...)>
{
  using Pointer = R(*)(Args...);
  using RetType = R;
  using ArgTypes = TypeList<Args...>;
  static constexpr std::size_t ArgCount = sizeof...(Args);
  template<std::size_t N>
    using NthArg = TypeListN<N,ArgTypes>;
  using Sequence = std::index_sequence_for<Args...>;
};

template<typename C, typename R, typename... Args>
struct FunctionTraits<R (C::*)(Args...)>
{
  using Pointer = R (C::*)(Args...);
  using RetType = R;
  using ArgTypes = TypeList<C,Args...>;
  static constexpr std::size_t ArgCount = sizeof...(Args) + 1;
  template<std::size_t N>
    using NthArg = TypeListN<N,ArgTypes>;
  using Sequence = std::index_sequence_for<C,Args...>;
};

template<size_t N, typename Context, typename...Ps, typename...PsRepr>
Context& extract_context(std::tuple<PsRepr...>& ps){
  auto& elt = std::get<N>(ps);
  using P = std::tuple_element_t<N,std::remove_reference_t<decltype(ps)>>::type;
  using Conversion = CamlConversion<P>;
  if constexpr (std::is_convertible_v<decltype(elt), Context&>) {
    return (Context&)elt;
  } else {
    if constexpr (HasContext<P>) {
      using EltContext = Conversion::Context;
      // CR smuenzel: should there be a reference?
      if constexpr (std::is_convertible_v<EltContext,Context>){
        return elt.context();
      } else {
        return extract_context<N+1,Context,Ps...>(ps);
      }
    } else {
      return extract_context<N+1,Context,Ps...>(ps);
    }
  }
}

template<
  auto F
, typename R = FunctionTraits<decltype(F)>::RetType
, typename Ps = FunctionTraits<decltype(F)>::ArgTypes
, typename Seq = FunctionTraits<decltype(F)>::Sequence
> struct
CallApi;

template< auto F, typename R, typename... Ps, size_t... Is>
requires (get_function_property<F,P_ImplicitFirstArgument>() == false)
struct CallApi<F,R,TypeList<Ps...>,std::index_sequence<Is...>>{
  static inline value invoke(decltype(Is, value{})... v_ps){
    constexpr auto releases_lock = get_function_property<F,P_ReleasesLock>();
    auto index_sequence = std::index_sequence<Is...>(); 
    std::tuple<typename CamlConversion<Ps>::OfValue::Representative...> p_psr(v_ps...);

    auto construct_return =
      [&p_psr](auto& ret) {
        using raw_ret = std::remove_reference_t<decltype(ret)>;
        if constexpr (HasContext<raw_ret>) {
          using Context = CamlConversion<raw_ret>::Context;
          auto context = extract_context<0, Context,Ps...>(p_psr);
          return CamlConversion<raw_ret>::ToValue::c(context, ret);
        } else { 
          return CamlConversion<raw_ret>::ToValue::c(ret);
        }
      };

    std::tuple<Ps&...> p_ps(p_psr);

    if constexpr (releases_lock) {
      // CR smuenzel: for some parameters, we might need to copy them
      caml_enter_blocking_section();
    };

    auto ret = invoke_seq_void(F, p_ps, index_sequence);

    if constexpr (releases_lock) {
      caml_leave_blocking_section();
    };

    return construct_return(ret);
  }
};

/////////////////////////////////////////////////////////////////////////////////////////
}
/////////////////////////////////////////////////////////////////////////////////////////
/// Simple types

DECL_API_TYPENAME(bool, bool);
DECL_API_TYPENAME(CppCaml::cstring, string);

/////////////////////////////////////////////////////////////////////////////////////////
/// Checks

template struct CppCaml::ApiTypename<std::optional<bool> >;
template struct CppCaml::ApiTypename<std::vector<bool> >;
template struct CppCaml::ApiTypename<std::pair<bool,std::vector<bool> >>;
