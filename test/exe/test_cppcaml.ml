open! Core
open Cppcaml_gen

external unit_x : unit -> unit = "caml_test_unit"

let () = ignore (unit_x)

let () =
  print_as_comment ()
