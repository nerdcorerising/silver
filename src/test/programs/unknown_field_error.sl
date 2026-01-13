# expect-error: Unknown field

class Point {
    x: public int;
    y: public int;
}

fn main() -> int {
    let p = alloc Point(1, 2);
    let z = p.z;
    return 0;
}
