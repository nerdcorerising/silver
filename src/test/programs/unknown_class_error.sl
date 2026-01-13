# expect-error: Unknown

fn main() -> int {
    let x = alloc NonExistentClass(0);
    return 0;
}
