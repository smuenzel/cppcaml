#include <cppcaml.h>
using namespace CppCaml;

template<uint64_t marker, typename T>
value section_to_list(T*start,T*stop){
  CAMLparam0();
  CAMLlocal3(v_ret, v_cur, v_prev);
  v_ret = Val_none;
  v_cur = Val_none;
  v_prev = Val_none;
  while(1){
    uint64_t* idx = (uint64_t*)start;
    uint64_t* sstop = (uint64_t*)stop;
    while(idx < sstop && *idx != marker){
      idx++;
    };
    start=(T*) idx;
    if(start >= stop) {
      if(v_prev == Val_none)
        CAMLreturn(Val_long(0));
      else{
        Store_field(v_prev,1,Val_none);
        CAMLreturn(v_ret);
      }
    } else {
      auto entry = (T*)start;
      v_cur = caml_alloc(2, 0);
      if(v_prev == Val_none){
        v_ret = v_cur;
      } else {
        Store_field(v_prev,1,v_cur);
      };
      Store_field(v_cur,0,entry->to_value());
      v_prev = v_cur;
      start = entry+1;
    }
  }
}

value ApiEntry::to_value(){
  CAMLparam0();
  CAMLlocal1(v_ret);
  switch(this->kind){
    case ApiEntryKind::Function:
      v_ret = caml_alloc_small(1,0);
      Store_field(v_ret,0,Val_unit);
      break;
    case ApiEntryKind::EnumMember:
      v_ret = caml_alloc_small(1,1);
      Store_field(v_ret,0,Val_unit);
      break;
    default:
        assert(false);
  };
  CAMLreturn(v_ret);
}

apireturn caml_get_api_registry(value){
  extern ApiEntry __start_caml_api_registry;
  extern ApiEntry __stop_caml_api_registry;
  return section_to_list<marker_value>(&__start_caml_api_registry,&__stop_caml_api_registry);
}
