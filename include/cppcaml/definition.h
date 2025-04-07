#if !defined(_CPPCAML_DEFINITION_H_)
#define _CPPCAML_DEFINITION_H_

namespace Cppcaml
{

  struct __attribute__((packed, aligned(8))) CamlFunctionRecord
    : public StaticCamlValue<0, 3>
  {
    const value v_name;
    const value v_args;
    const value v_dummy;
  };
  static_assert(CamlFunctionRecord::size == WoSizeC<CamlFunctionRecord>::v);
  static_assert(sizeof(CamlFunctionRecord) == sizeof(value) * (1 + 3));


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


  namespace XXX
  {
    /*
    const constexpr auto& x = Invoke<x>::Caml;

    extern "C" value y(value yy){
      return x(yy);
    }

    static const constexpr auto name = StaticCamlString<to_array("x")>();

    static const constexpr CamlFunctionRecord info
      __attribute__((used,retain,section("cppcaml_info_function")))
      =
    { .v_name = (value)name,
    };

    static const constexpr auto name2 = StaticCamlString<to_array("x2")>();
    static const constexpr CamlFunctionRecord info2
      __attribute__((used,retain,section("cppcaml_info_function")))
      =
    { .v_name = (value)name2,
    };
    */
  }



}

#endif
