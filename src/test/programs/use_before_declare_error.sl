# expect-error: Unknown variable

fn main() -> int {
    let y = x + 1;
    let x = 5;
    return y;
}
