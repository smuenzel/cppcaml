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

template<typename T>
value list_to_caml(const CamlLinkedList<T>* l){
  if(l == nullptr){
    return Val_long(0);
  } else {
    CAMLparam0();
    CAMLlocal3(v_l,v_ret,v_tmp);
    v_l = caml_alloc(2,0);
    v_ret = v_l;
    Store_field(v_l, 0, l->data.to_value());
    Field(v_l,1) = Val_unit;
    while(l->next){
      l = l->next;
      v_tmp = caml_alloc(2,0);
      Store_field(v_l, 1, v_tmp);
      v_l = v_tmp;
      Store_field(v_l,0, l->data.to_value());
      Field(v_l,1) = Val_unit;
    };
    CAMLreturn(v_ret);
  }
}

value ApiTypeDescription::to_value() const{
  CAMLparam0();
  CAMLlocal1(v_ret);
  v_ret = caml_alloc_small(2,0);

  Store_field(v_ret,0,caml_copy_string(this->name));
  Store_field(v_ret,1,CamlConversion<std::optional<bool>>::ToValue::c(this->conversion_allocates));

  CAMLreturn(v_ret);
}

value ApiFunctionDescription::to_value() const{
  CAMLparam0();
  CAMLlocal1(v_ret);
  v_ret = caml_alloc_small(5,0);

  Store_field(v_ret,0,this->return_type.to_value());
  Store_field(v_ret,1,list_to_caml(this->parameters));
  Store_field(v_ret,2,Val_bool(this->may_raise_to_ocaml));
  Store_field(v_ret,3,Val_bool(this->may_release_lock));
  Store_field(v_ret,4,Val_bool(this->has_implicit_first_argument));

  CAMLreturn(v_ret);
}

value ApiFunctionEntry::to_value() const{
  CAMLparam0();
  CAMLlocal1(v_ret);
  v_ret = caml_alloc_small(4,0);
  Store_field(v_ret,0,CamlConversion<cstring>::ToValue::c(this->wrapper_name));
  Store_field(v_ret,1,CamlConversion<cstring>::ToValue::c(this->function_name));
  Store_field(v_ret,2,CamlConversion<std::optional<cstring>>::ToValue::c(this->class_name));
  Store_field(v_ret,3,this->description.to_value());
  CAMLreturn(v_ret);
}

value ApiEntry::to_value() const{
  CAMLparam0();
  CAMLlocal1(v_ret);
  switch(this->kind){
    case ApiEntryKind::Function:
      v_ret = caml_alloc_small(1,0);
      Store_field(v_ret,0,this->as_function->to_value());
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
