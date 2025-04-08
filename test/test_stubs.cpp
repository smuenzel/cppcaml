
#include <cppcaml2.h>

struct my_rc {
  int count;

  my_rc() : count(1) {

  }

  void incr_count(){
    count++;
  }

  void decr_count(){
    count--;
    if(count <= 0)
      delete this;
  }
};


void x(int, int, const char*, uint8_t, int64_t)
{

};

using Invoke0 = Cppcaml::Invoke<x>;

using TypenameInt = Cppcaml::StaticCamlString<std::to_array("int")>;

static const constexpr auto name1 = Cppcaml::StaticCamlString<std::to_array("x_function")>();

static const constexpr auto args0 = Cppcaml::StaticCamlList<TypenameInt, TypenameInt>();
/*
static const constexpr auto args0 = Cppcaml::StaticCamlList<>();
*/

/*
static const constexpr Cppcaml::CamlFunctionRecord info1
__attribute__((used,retain,section("cppcaml_info_function")))
=
{ .v_name = (value)name1,
  .v_args = (value)args0,
  .v_return = Val_unit,
};

static const constexpr auto name2 = Cppcaml::StaticCamlString<std::to_array("y_function")>();

static const constexpr auto args1 = Cppcaml::OcamlTypenames<Invoke0::ArgTypes>();

static const constexpr Cppcaml::CamlFunctionRecord info2
__attribute__((used,retain,section("cppcaml_info_function")))
=
{ .v_name = (value)name2,
  .v_args = (value)args1,
  .v_return = Val_unit,
};
*/

extern "C" CAMLprim value caml_test_unit(value){
  return Val_unit;
}

/*
namespace XXX
{
  using Definition = Cppcaml::OcamlFunctionDefinition<std::to_array("my_function"), x>;
  CPPCAML_WRAPN(cppwrap_my_function, Definition::invoker, 5)
}
*/

DEF_CPPCAML(my_function, x, 5)
