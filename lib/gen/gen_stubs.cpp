#include <caml/memory.h>
#include <caml/mlvalues.h>
#include <caml/callback.h>

#include <cppcaml2.h>

using namespace Cppcaml;

extern "C" CAMLprim value cppcaml_iter_functions(value v_callback)
{
  extern CamlFunctionRecord __start_cppcaml_info_function;
  extern CamlFunctionRecord __stop_cppcaml_info_function;
  static_assert(sizeof(CamlFunctionRecord) == (1 + 3) * sizeof(value));
  CAMLparam1(v_callback);
  CAMLlocalresult(v_result);
  size_t count = ((uint8_t*)&__stop_cppcaml_info_function - (uint8_t*)&__start_cppcaml_info_function) / sizeof(CamlFunctionRecord);
  for(size_t i = 0; i< count; i++)
  {
    CamlFunctionRecord& record = (&__start_cppcaml_info_function)[i];
    v_result = caml_callback_res(v_callback, (value)record);
    (void)caml_get_value_or_raise(v_result);
  }
  CAMLreturn(Val_int(count));
}
