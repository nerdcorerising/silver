# Test complex arithmetic expressions

fn main() -> int {
    # Operator precedence
    let a = 2 + 3 * 4;
    if (a != 14) { return 1; }

    let b = (2 + 3) * 4;
    if (b != 20) { return 2; }

    # Chained comparisons with logical operators
    let x = 5;
    if (x > 0 && x < 10) { } else { return 3; }
    if (x < 0 || x > 4) { } else { return 4; }

    # Complex nested expression
    let c = ((10 + 5) * 2 - 8) / 2;
    if (c != 11) { return 5; }

    # Multiple operations
    let d = 100 - 50 + 25 - 10 + 5;
    if (d != 70) { return 6; }

    let e = 2 * 3 * 4 * 5;
    if (e != 120) { return 7; }

    return 50;
}
