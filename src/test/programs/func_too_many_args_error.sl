# expect-error: expects 1 argument(s) but got 3

fn double(n: int) -> int {
    return n * 2;
}

fn main() -> int {
    let x = double(1, 2, 3);
    return 0;
}
