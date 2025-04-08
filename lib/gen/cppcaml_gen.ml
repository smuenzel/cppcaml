open! Core

type function_record =
  { name : string
  ; cpp_name : string
  ; arguments : string list
  ; return : string
  } [@@deriving sexp]


external iter_functions : (function_record -> unit) -> int = "cppcaml_iter_functions"

external function_record : unit -> function_record = "__start_cppcaml_info_function"

let print_as_comment () =
  printf "\n";
  let count =
    iter_functions (printf !"(* %{sexp:function_record} *)\n")
  in
  printf !"(* %d functions *)\n" count


let print_externals () =
  printf "\n";
  let f { name; cpp_name; arguments; return; } =
    printf !"external %s\n  : %s -> %s\n  = \"%s\"\n\n"
      name
      (String.concat ~sep:" -> " arguments)
      return
      cpp_name
  in
  let count = iter_functions f in
  printf !"(* %d functions *)\n" count
