
(*$ open! Core
  open Cppcaml_gen
  (* dune cinaps currently doesn't support adding link flags, and we need -linkall,
     this is a workaround *)
  external unit_x : unit -> unit = "caml_test_unit"
    *)(*$*)

(*$
  let () =
    print_as_comment ()
*)(* y_function *)
(* x_function *)
(*$*)
