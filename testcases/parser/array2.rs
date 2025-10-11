fn main() {
    const C: usize = 1;
    let _: [u32; C] = [0; (_)]; // Inferred const.
    exit(0);
}
