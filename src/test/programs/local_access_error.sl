# expect-error: Function Math.helper is local and can only be called from within its namespace
namespace Math {
    local fn helper(x: int) -> int {
        return x * 2;
    }

    fn add(a: int, b: int) -> int {
        return a + b;
    }
}

fn main() -> int {
    let x = Math.helper(25);
    return x;
}
