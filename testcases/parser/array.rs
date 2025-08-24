fn main() {
    const C: usize = 1;
    let _: [u32; C] = [0; 1]; // Literal.
    let _: [u32; C] = [0; C]; // Constant item.
    let _: [u32; C] = [0; _]; // Inferred const.
    let _: [u32; C] = [0; (_)]; // Inferred const.
    let pair = ("a string", 2);
}
