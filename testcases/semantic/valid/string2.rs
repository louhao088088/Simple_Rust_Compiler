let x = String::from("hello");
let r1 = &x;      // &String
let r2 = &r1;     // &&String  
let r3 = &r2;     // &&&String

// All of these work through auto-deref
r1.len();  // &String -> String -> str -> len()
r2.len();  // &&String -> &String -> String -> str -> len()
r3.len();  // &&&String -> &&String -> &String -> String -> str -> len()