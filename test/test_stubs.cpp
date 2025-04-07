
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

static const constexpr auto name1 = Cppcaml::StaticCamlString<std::to_array("x_function")>();

static const constexpr Cppcaml::CamlFunctionRecord info1
__attribute__((used,retain,section("cppcaml_info_function")))
=
{ .v_name = (value)name1,
};

static const constexpr auto name2 = Cppcaml::StaticCamlString<std::to_array("y_function")>();

static const constexpr Cppcaml::CamlFunctionRecord info2
__attribute__((used,retain,section("cppcaml_info_function")))
=
{ .v_name = (value)name2,
};

extern "C" CAMLprim value caml_test_unit(value){
  return Val_unit;
}

