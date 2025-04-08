#if !defined(_CPPCAML_DEFINITION_H_)
#define _CPPCAML_DEFINITION_H_

namespace Cppcaml
{

  struct __attribute__((packed, aligned(1))) CamlFunctionRecord
    : public StaticCamlValue<0, 4>
  {
    const value v_name;
    const value v_cppname;
    const value v_args;
    const value v_return;
  };
  static_assert(CamlFunctionRecord::size == WoSizeC<CamlFunctionRecord>::v);
  static_assert(sizeof(CamlFunctionRecord) == sizeof(value) * (1 + 4));
  static_assert(alignof(CamlFunctionRecord) == 1);


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
    struct OcamlTypenames;

  template <typename... Args>
    struct OcamlTypenames<TypeList<Args...>> {
      static constexpr auto args = StaticCamlList<StaticCamlString<CamlType<Args>::typename_caml>...>();
      constexpr operator value() const { return (value)args; }
    };


  template<
    auto ocaml_function_name,
    auto f,
    auto... Properties
  > struct OcamlFunctionDefinition
  {
    using Invoker = Invoke<f, Properties...>;
    static const constexpr auto& invoker = Invoker::Caml;

    static const constexpr auto static_function_name =
      StaticCamlString<ocaml_function_name>();

    static const constexpr auto cpp_function_name =
      StaticCamlString<cat(to_array("ccwrap__"), ocaml_function_name)>();

    using ArgTypes = OcamlTypenames<typename Invoker::ArgTypes>;
    static constexpr auto return_type = StaticCamlString<CamlType<typename Invoker::ResultType>::typename_caml>();

    static const constexpr Cppcaml::CamlFunctionRecord info
      __attribute__((used,retain,section("cppcaml_info_function")))
      =
      { .v_name = (value)static_function_name,
        .v_cppname = (value)cpp_function_name,
        .v_args = (value)ArgTypes::args,
        .v_return = (value)return_type,
      };
  };

  extern "C" __attribute__((weak)) value cppcaml_retain_function(value v)
  {
    return v;
  }
}

#define DEF_CPPCAML(name, f, N, ...) \
  namespace Cppcaml::UserDefinitions::Def_##name { \
    using namespace Cppcaml; \
    using std::to_array; \
    using Definition = OcamlFunctionDefinition<to_array(#name), &f>; \
    CPPCAML_WRAPN(ccwrap__##name, Definition::invoker, N); \
  }

#define DEF_CPPCAML_S(name, N, ...) DEF_CPPCAML(name, name, N, ## __VA_ARGS__)

#endif
