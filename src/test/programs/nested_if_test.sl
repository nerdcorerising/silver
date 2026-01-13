# Test nested if/else statements

fn classify(n: int) -> int {
    if (n < 0) {
        if (n < -100) {
            return 1;
        } else {
            if (n < -10) {
                return 2;
            } else {
                return 3;
            }
        }
    } else {
        if (n == 0) {
            return 4;
        } else {
            if (n > 100) {
                return 5;
            } else {
                if (n > 10) {
                    return 6;
                } else {
                    return 7;
                }
            }
        }
    }
}

fn main() -> int {
    if (classify(0 - 200) != 1) { return 1; }
    if (classify(0 - 50) != 2) { return 2; }
    if (classify(0 - 5) != 3) { return 3; }
    if (classify(0) != 4) { return 4; }
    if (classify(200) != 5) { return 5; }
    if (classify(50) != 6) { return 6; }
    if (classify(5) != 7) { return 7; }

    return 50;
}
