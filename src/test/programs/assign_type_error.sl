# expect-error: Cannot assign type

fn main() -> int {
    let x: int = 5;
    x = "hello";
    return 0;
}
