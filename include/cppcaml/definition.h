#if !defined(_CPPCAML_DEFINITION_H_)
#define _CPPCAML_DEFINITION_H_

#include <fix8/conjure_enum.hpp>

namespace Cppcaml
{

  struct __attribute__((packed, aligned(32))) CamlFunctionRecord
    : public StaticCamlValue<0, 5>
  {
    const value v_name;
    const value v_cppname;
    const value v_args;
    const value v_return;
  };
  //static_assert(CamlFunctionRecord::size == WoSizeC<CamlFunctionRecord>::v);
  //static_assert(sizeof(CamlFunctionRecord) == sizeof(value) * (1 + 5));
  static_assert(alignof(CamlFunctionRecord) == 32);

  struct __attribute__((packed)) CamlEnumEntry
    : public StaticCamlValue<0, 2>
  {
    const value v_name;
    const value v_data;
  };

  struct __attribute__((packed, aligned(32))) CamlEnumRecord
    : public StaticCamlValue<0, 4>
  {
    const value v_name;
    const value v_cppname;
    const value v_is_bitflag;
    const value v_entries;
  };


#define CPPCAML_WRAP1(name, func) \
  extern "C" value name(value v0){ \
    return func(v0); \
  }

#define CPPCAML_WRAP2(name, func) \
  extern "C" value name(value v0, value v1){ \
    return func(v0, v1); \
  }

#define CPPCAML_WRAP3(name, func) \
  extern "C" value name(value v0, value v1, value v2){ \
    return func(v0, v1, v2); \
  }

#define CPPCAML_WRAP4(name, func) \
  extern "C" value name(value v0, value v1, value v2, value v3){ \
    return func(v0, v1, v2, v3); \
  }

#define CPPCAML_WRAP5(name, func) \
  extern "C" value name(value v0, value v1, value v2, value v3, value v4){ \
    return func(v0, v1, v2, v3, v4); \
  }

#define CPPCAML_WRAP6(name, func) \
  extern "C" value name(value v0, value v1, value v2, value v3, value v4, value v5){ \
    return func(v0, v1, v2, v3, v4, v5); \
  } \
  \
  extern "C" value name##_bytes(value* argv, int argn){ \
    return invoke_bytes<func>(argv, argn); \
  }

#define CPPCAML_WRAPN(name, func, N) \
  CPPCAML_WRAP##N(name, func)


  template <typename F>
    struct CamlTypenames;

  template <typename... Args>
    struct CamlTypenames<TypeList<Args...>> {
      static constexpr auto args
        = StaticCamlList<StaticCamlString<CamlType<Args>::typename_caml>...>();
      constexpr operator value() const { return (value)args; }
    };


  template<
    auto ocaml_function_name,
    auto f,
    auto... Properties
  > struct CamlFunctionDefinition
  {
    using Invoker = Invoke<f, Properties...>;
    static const constexpr auto& invoker = Invoker::Caml;

    static const constexpr auto static_function_name =
      StaticCamlString<ocaml_function_name>();

    static const constexpr auto cpp_function_name =
      StaticCamlString<cat(to_array("ccwrap__"), ocaml_function_name)>();

    using ArgTypes = CamlTypenames<typename Invoker::ArgTypes>;
    static constexpr auto return_type = StaticCamlString<CamlType<typename Invoker::ResultType>::typename_caml>();

    static const constexpr CamlFunctionRecord info
      __attribute__((used,retain,section("cppcaml_info_function")))
      =
      { .v_name = (value)static_function_name,
        .v_cppname = (value)cpp_function_name,
        .v_args = (value)ArgTypes::args,
        .v_return = (value)return_type,
      };
  };


  // CR smuenzel: there are a lot of intermediate values here that could maybe be eliminated
  template<typename E>
    struct CamlEnumDefinition
  {
    static const constexpr auto enum_name_fix8 = FIX8::conjure_type<E>::name;
      
    static const constexpr auto enum_name =
      StaticCamlString<make_string_view_array<enum_name_fix8.size() - 1>(enum_name_fix8)>();

    static const constexpr auto c_names = FIX8::conjure_enum<E>::unscoped_names;
    static const constexpr auto c_values = FIX8::conjure_enum<E>::values;
    static const constexpr size_t count = std::tuple_size_v<decltype(c_names)>;
    static const constexpr auto seq = std::make_index_sequence<count>();

    static const constexpr auto t_names = std::tuple_cat(c_names);

    template<size_t... I>
      static const constexpr auto convert_names(std::index_sequence<I...> seq)
      {
         return std::make_tuple(make_string_view_array<std::get<I>(t_names).size()>(std::get<I>(t_names))...);
      }

    static const constexpr auto tt_names = convert_names(seq);

    template<size_t... I>
      static const constexpr auto names_as_static(std::index_sequence<I...> seq)
      {
         return std::tuple(StaticCamlString<std::get<I>(tt_names)>()...);
      }

    static const constexpr auto scs_names = names_as_static(seq);

    template<size_t... I>
      static const constexpr auto make_entries(std::index_sequence<I...> seq)
      {
        return std::array<CamlEnumEntry, count>{(CamlEnumEntry){.v_name = (value)std::get<I>(scs_names), .v_data = Val_long(std::get<I>(c_values))}...};
      }

    static const constexpr auto entries = make_entries(seq);

    template<size_t... I>
      static const constexpr auto make_value_entries(std::index_sequence<I...> seq)
      {
        return std::array<value, count>{(value)std::get<I>(entries)...};
      }

    static const constexpr auto v_entries = make_value_entries(seq);

    static const constexpr auto v_array_entries = StaticCamlArrayDirect<count>{ .values = v_entries };

    static const constexpr CamlEnumRecord info
      __attribute__((used,retain,section("cppcaml_info_enum")))
      =
      {
        .v_name = (value)enum_name,
        .v_cppname = (value)enum_name,
        .v_is_bitflag = Val_bool(false),
        .v_entries = (value)v_array_entries,
      };

  };

  extern "C" __attribute__((weak)) value cppcaml_retain_function(value v)
  {
    return v;
  }
}

#define DEF_CPPCAML(name, f, N, ...) \
  namespace Cppcaml::UserDefinitions::Fun_##name { \
    using namespace Cppcaml; \
    using std::to_array; \
    using Definition = CamlFunctionDefinition<to_array(#name), &f>; \
    CPPCAML_WRAPN(ccwrap__##name, Definition::invoker, N); \
  }

#define DEF_CPPCAML_S(name, N, ...) DEF_CPPCAML(name, name, N, ## __VA_ARGS__)

#define DEF_CPPCAML_ENUM(name) \
  namespace Cppcaml::UserDefinitions::Enum_##name { \
    using namespace Cppcaml; \
    using Definition = CamlEnumDefinition<name>; \
    static const constexpr auto& info = Definition::info; \
  }

#endif
