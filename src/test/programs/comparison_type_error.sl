# expect-error: Type mismatch

fn main() -> int {
    if (5 > "hello") {
        return 1;
    }
    return 0;
}
