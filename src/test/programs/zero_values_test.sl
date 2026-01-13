# Test operations with zero values

fn main() -> int {
    # Zero arithmetic
    let a = 0 + 0;
    if (a != 0) { return 1; }

    let b = 10 - 10;
    if (b != 0) { return 2; }

    let c = 0 * 100;
    if (c != 0) { return 3; }

    let d = 0 / 5;
    if (d != 0) { return 4; }

    # Zero comparisons
    if (0 == 0) { } else { return 5; }
    if (0 >= 0) { } else { return 6; }
    if (0 <= 0) { } else { return 7; }

    # Zero in loop
    let count = 0;
    let i = 0;
    while (i < 0) {
        count = count + 1;
        i = i + 1;
    }
    if (count != 0) { return 8; }

    return 50;
}
