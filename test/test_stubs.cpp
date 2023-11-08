
#include <cppcaml.h>

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

using namespace CppCaml;

template<> struct CppCaml::AutoConversion<struct my_rc*>{
  static const constexpr AutoConversionKind kind = AutoConversionKind::SharedPointer;

};

bool apix(bool x, bool){
  return x+1;
}

DECL_API_TYPENAME(int, int);
F_PROP(apix,ReleasesLock, false);

CPPCAML_REGISTER_FUN(example
    , .wrapper_name = "caml_hello"
    , .function_name = "apix"
    , .description = CppCaml::make_function_description<apix>()
    );

apireturn caml_hello(value a, value b){
  return CppCaml::CallApi<apix>::invoke(a, b);
}


apireturn caml_test_unit(value){
  return Val_unit;
}
