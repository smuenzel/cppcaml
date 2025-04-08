
(*$ open! Core
  open Cppcaml_gen
  (* dune cinaps currently doesn't support adding link flags, and we need -linkall,
     this is a workaround. It seems to ignore the flags currently *)
  external unit_x : unit -> unit = "caml_test_unit"
    *)(*$*)

(*$
  let () =
    print_as_comment ()
*)
(* ((name my_function) (cpp_name ccwrap__my_function)
 (arguments (int int string uint8 int64)) (return unit)) *)
(* 1 functions *)
(*$*)

(*$
  let () =
    print_externals ()
*)
external my_function
  : int -> int -> string -> uint8 -> int64 -> unit
  = "ccwrap__my_function"

(* 1 functions *)
(*$*)
