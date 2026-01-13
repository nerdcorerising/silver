# Test functions with minimal/no operations

fn doNothing() {
}

fn returnOnly() -> int {
    return 42;
}

fn emptyIf(x: int) -> int {
    if (x > 0) {
    }
    return x;
}

fn main() -> int {
    doNothing();

    if (returnOnly() != 42) { return 1; }
    if (emptyIf(5) != 5) { return 2; }

    return 50;
}
