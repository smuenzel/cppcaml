open! Core

type function_description =
  { return_type : unit
  ; parameters : unit
  ; may_raise_to_ocaml : bool
  ; may_release_lock : bool
  ; has_implicit_first_argument : bool
  } [@@deriving sexp]

type function_entry =
  { wrapper_name : string
  ; function_name : unit
  ; class_name : unit
  ; description : function_description
  } [@@deriving sexp]

type enum_entry = unit
[@@deriving sexp]

type api_entry =
  | Function of function_entry
  | Enum of enum_entry
[@@deriving sexp]

external get_api_registry : unit -> api_entry list = "caml_get_api_registry"


let print_as_comment () =
  let registry = get_api_registry () in
  Printf.printf "(*\n";
  List.iter registry
    ~f:(fun entry ->
        [%sexp_of: api_entry] entry
        |> print_s
      );
  Printf.printf "*)\n";
