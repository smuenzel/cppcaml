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


module Casify = struct
  let make (s : string) (c : Capitalization.t) =
    let as_list = String.to_list s in
    let (rev_acc_curr, rev_acc_other), _ =
      List.fold as_list
        ~init:(([],[]), None)
        ~f:(fun ((rev_acc_curr, rev_acc_other), prev_is_lower) c ->
            let is_lower = Char.is_lowercase c in
            let is_upper = Char.is_uppercase c in
            let should_split =
              match prev_is_lower with
              | None -> false
              | Some prev_is_lower -> prev_is_lower && is_upper
            in
            if should_split
            then (([], (c :: rev_acc_curr) :: rev_acc_other), None)
            else ((c :: rev_acc_curr, rev_acc_other), Some is_lower))
    in
    let words =
      List.filter_map
        (rev_acc_curr :: rev_acc_other)
        ~f:(function
            | [] -> None
            | list -> List.rev list |> String.of_char_list |> Some
          )
    in
    Capitalization.apply_to_words c words
end

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

let enum_entry_name ~module_name ~(entry : enum_entry) =
  let module_name_snake = Casify.make module_name Snake_case in
  let oname = Casify.make entry.name Snake_case in
  String.chop_prefix_if_exists oname ~prefix:(module_name_snake ^ "_")

let print_enum (er : enum_record) =
  let module_name = Casify.make er.name Capitalized_snake_case in
  printf "\n";
  printf "module %s : sig\n" module_name;
  printf "  type t [@@immediate]\n\n";
  printf "  val to_int : t -> int\n";
  printf "  val of_int : int -> t\n\n";
  Array.iter er.entries ~f:(fun entry ->
      let oname = enum_entry_name ~module_name ~entry in
      printf "  val %s : t\n" oname;
    );
  printf "end = struct\n";
  printf "  type t = int\n\n";
  printf "  let to_int x = x\n";
  printf "  let of_int x = x\n\n";
  Array.iter er.entries ~f:(fun entry ->
      let oname = enum_entry_name ~module_name ~entry in
      printf "  let %s = %i\n" oname entry.value;
    );
  printf "end\n";
  ()

let print_enums () =
  printf "\n";
  let count = iter_enums print_enum in
  printf !"\n\n(* %d enums *)\n" count

