fn main() -> int {
    # Arithmetic operators
    let a = 10 + 5;
    let b = 10 - 3;
    let c = 6 * 4;
    let d = 20 / 4;

    # Verify arithmetic results
    if (a != 15) { return 1; }
    if (b != 7) { return 2; }
    if (c != 24) { return 3; }
    if (d != 5) { return 4; }

    # Relational comparisons
    if (10 > 5) { } else { return 5; }
    if (5 < 10) { } else { return 6; }
    if (5 >= 5) { } else { return 7; }
    if (5 <= 5) { } else { return 8; }
    if (5 == 5) { } else { return 9; }
    if (5 != 6) { } else { return 10; }

    # Logical operators
    if (1 == 1 && 2 == 2) { } else { return 11; }
    if (1 == 2 || 2 == 2) { } else { return 12; }

    # Negative numbers
    let neg = 0 - 10;
    if (neg != 0 - 10) { return 13; }

    # Combined expression
    let result = (10 + 5) * 2;
    if (result != 30) { return 14; }

    return 50;
}
