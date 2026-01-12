# expect-error: already declared

fn main() -> int {
    let x = 5;
    let x = 10;
    return x;
}
