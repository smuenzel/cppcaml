#include <caml/memory.h>
#include <caml/mlvalues.h>
#include <caml/callback.h>

#include <cppcaml.h>

using namespace Cppcaml;

extern "C" CamlFunctionRecord __attribute__((weak)) __start_cppcaml_info_function;
extern "C" CamlFunctionRecord __attribute__((weak)) __stop_cppcaml_info_function;

extern "C" CAMLprim value cppcaml_iter_functions(value v_callback)
{
  extern CamlFunctionRecord __start_cppcaml_info_function;
  extern CamlFunctionRecord __stop_cppcaml_info_function;
  CAMLparam1(v_callback);
  CAMLlocalresult(v_result);
  ssize_t count = ((uint8_t*)&__stop_cppcaml_info_function - (uint8_t*)&__start_cppcaml_info_function) / sizeof(CamlFunctionRecord);
  for(ssize_t i = count - 1; i >= 0; i--)
  {
    CamlFunctionRecord& record = (&__start_cppcaml_info_function)[i];
    v_result = caml_callback_res(v_callback, (value)record);
    (void)caml_get_value_or_raise(v_result);
  }
  CAMLreturn(Val_int(count));
}

extern "C" CamlEnumRecord __attribute__((weak)) __start_cppcaml_info_enum;
extern "C" CamlEnumRecord __attribute__((weak)) __stop_cppcaml_info_enum;

extern "C" CAMLprim value cppcaml_iter_enums(value v_callback)
{
  extern CamlEnumRecord __start_cppcaml_info_enum;
  extern CamlEnumRecord __stop_cppcaml_info_enum;
  CAMLparam1(v_callback);
  CAMLlocalresult(v_result);
  ssize_t count = ((uint8_t*)&__stop_cppcaml_info_enum - (uint8_t*)&__start_cppcaml_info_enum) / sizeof(CamlEnumRecord);
  for(ssize_t i = count - 1; i >= 0; i--)
  {
    CamlEnumRecord& record = (&__start_cppcaml_info_enum)[i];
    v_result = caml_callback_res(v_callback, (value)record);
    (void)caml_get_value_or_raise(v_result);
  }
  CAMLreturn(Val_int(count));
}
