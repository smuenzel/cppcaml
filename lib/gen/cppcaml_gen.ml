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

module With_prefix = struct
  module Module_contents = struct
    type t =
      { functions : function_record String.Map.t
      ; enums : enum_record String.Map.t
      ; types : string String.Map.t
      } [@@deriving sexp]

    let empty = { functions = String.Map.empty; enums = String.Map.empty; types = String.Map.empty }
  end
  
  module Keychainable_string_list = struct
    include Trie.Keychainable.Of_list(String)

    let sexp_of_t (t : t) =
      [%sexp_of: string list] (List.rev t)
  end

  type trie =
    ( Keychainable_string_list.t
    , Module_contents.t
    , Keychainable_string_list.keychain_description [@sexp.opaque]
    ) Trie.t
  [@@deriving sexp_of]

  type t =
    { type_alias : string String.Map.t
    } [@@deriving sexp]

  let make_type_aliases (trie : trie) =
    Trie.foldi
      trie
      ~init:String.Map.empty
      ~f:(fun acc ~keychain ~data ->
          Map.fold data.types ~init:acc ~f:(fun ~key ~data acc ->
              let target_type =
                let module_part =
                  List.rev_map ~f:String.capitalize keychain
                  |> String.concat ~sep:"."
                in
                let type_part = String.lowercase key in
                module_part ^ "." ^ type_part
              in
              Map.add_exn acc ~key:data ~data:target_type
            )
        )

  let rec create_trie ~acc_parent ~acc_current source target =
    let target =
      match Trie.datum source with
      | None -> target
      | Some kind ->
        let name = String.concat ~sep:"_" (List.rev acc_current) in
        Trie.change target
          (Trie.Keychainable.keychain_of_rev_keys Keychainable_string_list.keychainable
             acc_parent)
          ~f:(fun current ->
             let current = Option.value ~default:Module_contents.empty current in
             let functions =
               match kind with
               | `Function fr -> 
                 Map.set current.functions ~key:name ~data:fr
               | _ -> current.functions
             in
             let enums =
               match kind with
               | `Enum er ->
                 Map.set current.enums ~key:name ~data:er
               | _ -> current.enums
             in
             let types =
               match kind with
               | `Type t ->
                 Map.set current.types ~key:name ~data:t
               | _ -> current.types
             in
             Some { Module_contents.functions; enums; types }
          )
    in
    let source_subtries = Trie.tries source in
    let length = Map.length source_subtries in
    let acc_parent, acc_current =
      match length with
      | i when i > 1 ->
        let acc_parent = acc_parent @ [ String.concat ~sep:"_" acc_current ] in
        let acc_current = [] in
        acc_parent, acc_current
      | _ -> acc_parent, acc_current
    in
    Map.fold source_subtries ~init:target ~f:(fun ~key ~data target ->
        create_trie ~acc_parent ~acc_current:(key :: acc_current) data target
      )

  let create_trie source =
    create_trie
      ~acc_parent:[]
      ~acc_current:[]
      source
      (Trie.empty Keychainable_string_list.keychainable)
end

let rec print_trie ~indent ~(acc : string list) trie =
  begin match Trie.datum trie with
  | None -> ()
  | Some kind ->
    printf "%sdata @ %s: %s\n" indent
      (String.concat ~sep:"_" (List.rev acc))
      (match kind with `Function _ -> "function" | `Enum _ -> "enum" | `Type _ -> "type")
  end;
  let tries = Trie.tries trie in
  match Map.length tries with
  | 0 -> ()
  | i when i > 1 ->
    printf "%ssplit @ %s\n" indent (String.concat ~sep:"_" (List.rev acc));
    let indent = indent ^ "  " in
    Map.iteri tries ~f:(fun ~key ~data ->
        print_trie ~indent ~acc:[ key ] data
      )
  | _ ->
    Map.iteri tries ~f:(fun ~key ~data ->
        let acc = key :: acc in
        print_trie ~indent ~acc data
      )

let print_prefix types =
  printf "(*\n";
  let module Keychainable = Trie.Keychainable.Of_list(String) in
  let trie =
    List.fold types
      ~init:(Trie.empty Keychainable.keychainable)
      ~f:(fun trie typ ->
          let keychain = (String.split ~on:'_' typ) @ [ "t" ] in
          Trie.add_exn trie ~keychain ~data:(`Type typ)
        )
  in
  let trie = ref trie in
  let _count =
    iter_functions
      (fun fr ->
         let keychain = String.split ~on:'_' fr.name in
         trie := Trie.add_exn !trie ~keychain ~data:(`Function fr);
      )
  in
  let _count =
    iter_enums
      (fun er ->
         let keychain = String.split ~on:'_' er.name in
         trie := Trie.add_exn !trie ~keychain ~data:(`Enum er);
      )
  in
  print_trie ~indent:"" ~acc:[] !trie;
  printf "*)\n";
  printf "(*\n";
  let t' = With_prefix.create_trie !trie in
  print_s ([%sexp_of: With_prefix.trie] t');
  printf "*)\n";
  printf "(*\n";
  let type_alias = With_prefix.make_type_aliases t' in
  print_s ([%sexp_of: With_prefix.t] { type_alias });
  printf "*)\n";

  ()



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
  let simple_type_name = Casify.make er.name Snake_case in
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
  printf "end\n\n";
  printf "type %s = %s.t\n\n" simple_type_name module_name;
  ()

let print_enums () =
  printf "\n";
  let count = iter_enums print_enum in
  printf !"\n\n(* %d enums *)\n" count

