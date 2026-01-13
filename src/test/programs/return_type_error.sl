# expect-error: Return type mismatch

fn getString() -> string {
    return 42;
}

fn main() -> int {
    let s = getString();
    return 0;
}
