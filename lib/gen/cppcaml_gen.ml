open! Core

type function_record =
  { name : string
  ; arguments : string list
  } [@@deriving sexp]


external iter_functions : (function_record -> unit) -> int = "cppcaml_iter_functions"

(*
external function_record : function_record = "__start_cppcaml_info_function"
   *)

let print_as_comment () =
  printf "\n";
  let count =
    iter_functions (printf !"(* %{sexp:function_record} *)\n")
  in
  printf !"(* %d functions *)\n" count
