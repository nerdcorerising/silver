namespace Math {
    local fn helper(x: int) -> int {
        return x * 2;
    }

    fn add(a: int, b: int) -> int {
        return a + b;
    }

    fn subtract(a: int, b: int) -> int {
        return a - b;
    }

    fn addThenDouble(a: int, b: int) -> int {
        let sum = add(a, b);
        return helper(sum);
    }
}

namespace Math.Advanced {
    fn square(x: int) -> int {
        return x * x;
    }
}

fn main() -> int {
    let doubled = Math.addThenDouble(10, 15);
    let sum = Math.add(25, 25);
    return sum;
}
