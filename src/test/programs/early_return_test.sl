# Test early returns in various contexts

fn findFirst(a: int, b: int, c: int, target: int) -> int {
    if (a == target) {
        return 1;
    }
    if (b == target) {
        return 2;
    }
    if (c == target) {
        return 3;
    }
    return 0;
}

fn loopWithEarlyReturn(limit: int) -> int {
    let i = 0;
    while (i < 100) {
        if (i == limit) {
            return i;
        }
        i = i + 1;
    }
    return 0 - 1;
}

fn main() -> int {
    if (findFirst(5, 10, 15, 10) != 2) { return 1; }
    if (findFirst(5, 10, 15, 99) != 0) { return 2; }

    if (loopWithEarlyReturn(25) != 25) { return 3; }

    return 50;
}
