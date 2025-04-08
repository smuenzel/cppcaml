
(*$ open! Core
  open Cppcaml_gen
  (* dune cinaps currently doesn't support adding link flags, and we need -linkall,
     this is a workaround *)
  external unit_x : unit -> unit = "caml_test_unit"
    *)(*$*)

(*$
  let () =
    print_as_comment ()
*)
(* ((name my_function) (arguments (int int string uint8 int64))) *)
(* 1 functions *)
(*$*)
