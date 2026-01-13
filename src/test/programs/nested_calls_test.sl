# Test nested function and method calls

fn double(n: int) -> int {
    return n * 2;
}

fn triple(n: int) -> int {
    return n * 3;
}

class Calc {
    value: public int;

    fn add(n: int) -> int {
        this.value = this.value + n;
        return this.value;
    }

    fn multiply(n: int) -> int {
        this.value = this.value * n;
        return this.value;
    }
}

fn main() -> int {
    # Nested function calls
    let x = double(triple(2));
    if (x != 12) { return 1; }

    # Nested method calls
    let c = alloc Calc(5);
    let y = c.multiply(c.add(3));
    if (y != 64) { return 2; }

    return 50;
}
