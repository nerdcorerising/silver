# Test negative number operations

fn abs(n: int) -> int {
    if (n < 0) {
        return 0 - n;
    }
    return n;
}

fn main() -> int {
    # Negative arithmetic
    let a = 0 - 5;
    if (a != 0 - 5) { return 1; }

    let b = a + 10;
    if (b != 5) { return 2; }

    let c = a * 2;
    if (c != 0 - 10) { return 3; }

    let d = a * a;
    if (d != 25) { return 4; }

    # Negative comparisons
    if (0 - 5 < 0) { } else { return 5; }
    if (0 - 10 < 0 - 5) { } else { return 6; }

    # Absolute value
    if (abs(0 - 42) != 42) { return 7; }
    if (abs(42) != 42) { return 8; }

    return 50;
}
