fn multiply_matrices() -> [[i32; 3]; 3] {
    let a: [[i32; 3]; 3] = [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
    ];
    let b: [[i32; 3]; 3] = [
        [9, 8, 7],
        [6, 5, 4],
        [3, 2, 1],
    ];
    let result = [[0; 3]; 3];
    let mut i = 0;
    while i < 3 {
        let mut j = 0;
        while j < 3 {
            let mut k = 0;
            while k < 3 {
                let value = a[i][k] * b[k][j];
                result[i][j] = result[i][j] + value;
                k += 1;
            }
            j += 1;
        }
        i += 1;
    }
    result
}

fn trace_diagonal(m: [[i32; 3]; 3]) -> i32 {
    let mut s = 0;
    let mut n = 0;
    while n < 3 {
        s = s + m[n][n];
        n += 1;
    }
    s
}

fn accumulate_edges(m: [[i32; 3]; 3]) -> i32 {
    let mut t = 0;
    let mut r = 0;
    while r < 3 {
        t = t + m[r][0];
        t = t + m[r][2];
        r += 1;
    }
    let mut c = 0;
    while c < 3 {
        t = t + m[0][c];
        t = t + m[2][c];
        c += 1;
    }
    t
}

fn main() {
    let m = multiply_matrices();
    let d = trace_diagonal(m);
    let e = accumulate_edges(m);
    let c = d + e;
    if c > 0 {
        let _s = 1;
    } else {
        let _s = 0;
    }
}
