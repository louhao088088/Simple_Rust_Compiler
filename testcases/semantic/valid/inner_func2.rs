fn main() {
    struct Agent {
        id: i32,
        x: i32,
        y: i32,
        energy: i32,
        target_x: i32,
        target_y: i32,
        state: i32, 
    }

    let mut world: [[i32; 64]; 64] = [[0; 64]; 64];
    let mut agents: [Agent; 32] = [Agent {
        id: 0,
        x: 0,
        y: 0,
        energy: 0,
        target_x: 0,
        target_y: 0,
        state: 0,
    }; 32];

    fn init_world() {
        find_new_target();
    }

    fn find_new_target() {
        find_new_target();
    }
    exit(0);
}
