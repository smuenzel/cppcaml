#if !defined(_CPPCAML_VALUES_H_)
#define _CPPCAML_VALUES_H_

#include <memory>

#include <fix8/conjure_enum.hpp>
#include <fix8/conjure_type.hpp>

#include <caml/custom.h>

#include <cppcaml/utils.h>

namespace Cppcaml {

template<typename T>
struct CamlCustomValue
{
  static void finalize(value v){
    T* t = (T*) Data_custom_val(v);
    t->~T();
  }

  static int compare(value v1, value v2) {
    T* t1 = (T*) Data_custom_val(v1);
    T* t2 = (T*) Data_custom_val(v2);
    const auto cmp{*t1 <=> *t2};
    return cmp < 0 ? -1 : (cmp > 0 ? 1 : 0);
  }

  static intnat hash(value v)
  {
    return std::hash<T>()(*((T*) Data_custom_val(v)));
  }

  static constexpr const struct custom_operations ops = {
    .identifier = FIX8::conjure_type<T>::name.c_str(),
    .finalize = &finalize,
    .compare = &compare,
    .hash = &hash,
    .serialize = custom_serialize_default,
    .deserialize = custom_deserialize_default,
    .compare_ext = custom_compare_ext_default,
    .fixed_length = 0,
  };

  template<typename... Args>
  static value allocate(Args&&... args) {
    /* CR smuenzel: figure out used and max */
    value v = caml_alloc_custom(&ops, sizeof(T), 1, 100);
    void* vp = Data_custom_val(v);
    new (vp) T(std::forward<Args>(args)...);
    return v;
  }

  static T& from_value(value v) {
    auto custom_ops_from_v = Custom_ops_val(v);
    if(custom_ops_from_v != &ops) {
      caml_failwith_printf("Cannot convert %s to %s",
          custom_ops_from_v->identifier, ops.identifier);
    }
    return *((T*) Data_custom_val(v));
  }

};

template<typename T>
struct SharedPtrCustomValue : public CamlCustomValue<std::shared_ptr<T>> {};

}

#endif
