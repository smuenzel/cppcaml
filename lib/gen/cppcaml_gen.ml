open! Core

type function_record =
  { name : string
  ; cpp_name : string
  ; arguments : string list
  ; return : string
  } [@@deriving sexp]

type enum_entry =
  { name : string
  ; value : int
  } [@@deriving sexp]

type enum_record =
  { name : string
  ; cpp_name : string
  ; is_bitflag : bool
  ; entries : enum_entry array
  } [@@deriving sexp]

external iter_functions : (function_record -> unit) -> int = "cppcaml_iter_functions"
external iter_enums : (enum_record -> unit) -> int = "cppcaml_iter_enums"

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

let enum_print_as_comment () =
  printf "\n";
  let count =
    iter_enums
      (*
      (fun { cpp_name; entries; _ } -> printf "(* %s (%i) %s *)\n" cpp_name (Array.length entries) (entries.(2).name))
        *)
      (printf !"(* %{sexp:enum_record} *)\n")
  in
  printf !"(* %d enums *)\n" count
