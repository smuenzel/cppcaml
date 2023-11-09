
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

inline bool apix(bool x, bool y){
  return x+y;
}

DECL_API_TYPENAME(int, int);

F_PROP(apix,ReleasesLock, false);

CPPCAML_REGISTER_FUN(example
    , .wrapper_name = "caml_hello"
    , .function_name = "apix"
    , .description = CppCaml::make_function_description<apix>()
    );

#define CAT(A,B) A ## B

#define REP1(F) CAT(F,0)
#define REP2(F) CAT(F,0), CAT(F,1)
#define REP3(F) CAT(F,0), CAT(F,1), CAT(F,2)
#define REP4(F) CAT(F,0), CAT(F,1), CAT(F,2), CAT(F,4)

#define VALUE(X) value X

#define API_IMPL(WRAPPER_NAME,API_NAME,REP) \
apireturn WRAPPER_NAME (REP(VALUE(v))){ \
  return CppCaml::CallApi<API_NAME>::invoke(REP(v)); \
}

API_IMPL(caml_hello,apix,REP2)


apireturn caml_test_unit(value){
  return Val_unit;
}

void print_x(){
  printf("e\n");
}

struct MyClass : private boost::noncopyable {
  int x;

  void incr() { 
    x++;
  }

  int get_x() { return x; }

  MyClass(int x) : x(x) {}

  MyClass(MyClass&& rhs) = default;
};

DECL_API_TYPENAME(MyClass,myclass);

template<> struct CppCaml::CamlConversion<MyClass> {
  struct ToValue {
    static const bool allocates = false;

    static inline value c(const MyClass&m){
      return Val_int(m.x);
    }
  };

  struct OfValue {
    struct Representative {
      MyClass m;
      operator MyClass&() { return m; }

      Representative(value v) : m(Int_val(v)){

      }
    };
  };
};

template<>
struct CppCaml::CamlConversion<int> {
  struct ToValue {
    static const bool allocates = false;

    static inline value c(const int& b){
      return Val_int(b);
    }
  };

  struct OfValue {
    struct Representative {
      int v;
      operator int&() { return v; }
    };

    static inline Representative c(value v){
      return { .v = Int_val(v) };
    }
  };
};


apireturn caml_myclass_incr(value mc){
  return CppCaml::CallApi<&MyClass::incr>::invoke(mc);
}

apireturn caml_myclass_get_x(value mc){
  return CppCaml::CallApi<&MyClass::get_x>::invoke(mc);
}

