fn take_str(s: &str) {}
let string = String::from("hello");
let string_ref = &string;
take_str(string_ref); // &String -> &str

// Let bindings with type annotations
let s: &str = &String::from("hello"); // &String -> &str