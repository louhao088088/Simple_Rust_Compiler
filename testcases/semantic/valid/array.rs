fn main() {
    const C: usize = 1;
    let _: [u32; 4] = [0; 1]; // Literal.
    let _: [u32; 5] = [0; C]; // Constant item.
    let _: [u32; 4] = [0; _]; // Inferred const.
    let _: [u32; C] = [0; (_)]; // Inferred const.
}
