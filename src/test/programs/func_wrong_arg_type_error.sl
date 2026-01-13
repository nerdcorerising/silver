# expect-error: expects type int but got string

fn double(n: int) -> int {
    return n * 2;
}

fn main() -> int {
    let x = double("hello");
    return 0;
}
