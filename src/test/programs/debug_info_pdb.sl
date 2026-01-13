# expect-symbols: main, helper

fn helper(x: int) -> int {
    return x * 2;
}

fn main() -> int {
    let a = 10;
    let b = helper(a);
    return 50;
}
