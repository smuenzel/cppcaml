open! Core

type function_record =
  { name : string
  } [@@deriving sexp]


external iter_functions : (function_record -> unit) -> unit = "cppcaml_iter_functions"

external function_record : function_record = "__start_cppcaml_info_function"

let print_as_comment () =
  iter_functions (fun { name } -> printf "(* %s *)\n" name)
