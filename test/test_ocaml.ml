
(*$ open! Core
  open Cppcaml_gen
  (* dune cinaps currently doesn't support adding link flags, and we need -linkall,
     this is a workaround *)
  external unit_x : unit -> unit = "caml_test_unit"
    *)(*$*)

(*$
  let () =
    print_as_comment ()
*)(* ((name y_function) (arguments (int int))) *)
(* 2 functions *)
(*$*)
