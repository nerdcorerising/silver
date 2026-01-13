# expect-error: expects 2 argument(s) but got 1

namespace Math {
    fn add(a: int, b: int) -> int {
        return a + b;
    }
}

fn main() -> int {
    let x = Math.add(5);
    return 0;
}
