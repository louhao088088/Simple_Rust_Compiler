
struct GraphNode {
    id: i32,
    visited: bool,
    neighbors: [i32; 5],
    neighbor_count: i32,
}

impl GraphNode {
    fn new(node_id: i32) -> GraphNode {
        GraphNode {
            id: node_id,
            visited: false,
            neighbors: [0; 5],
            neighbor_count: 0,
        }
    }
}

fn main() {
    exit(0);
}
