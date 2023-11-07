open! Core

type function_entry = unit
[@@deriving sexp]
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
