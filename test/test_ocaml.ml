
(*$ open! Core
  open Cppcaml_lib
  (* dune cinaps currently doesn't support adding link flags, and we need -linkall,
     this is a workaround *)
  external unit_x : unit -> unit = "caml_test_unit"
    *)(*$*)

(*$
  let () =
    print_as_comment ()
*)(*
(Function
 ((wrapper_name hello) (function_name ()) (class_name ())
  (description
   ((return_type ((name bool) (conversion_allocates (false))))
    (parameters ()) (may_raise_to_ocaml true) (may_release_lock true)
    (has_implicit_first_argument false)))))
*)
(*$*)
