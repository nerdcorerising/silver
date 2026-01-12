fn main() -> int {
    let f1 = 3.14;
    let f2 = 2.0;
    let f3 = f1 + f2;

    # Float comparisons (approximate)
    if (f3 > 5.0) { } else { return 1; }
    if (f3 < 6.0) { } else { return 2; }

    # Float multiplication
    let f4 = 2.5 * 4.0;
    if (f4 > 9.9) { } else { return 3; }
    if (f4 < 10.1) { } else { return 4; }

    # Float division
    let f5 = 10.0 / 4.0;
    if (f5 > 2.4) { } else { return 5; }
    if (f5 < 2.6) { } else { return 6; }

    # Cast int to float
    let i = 5;
    let f6 = (float)i;
    if (f6 > 4.9) { } else { return 7; }
    if (f6 < 5.1) { } else { return 8; }

    return 50;
}
