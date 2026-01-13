# Test member access and method calls on objects

class Counter {
    value: public int;
    step: public int;

    fn getValue() -> int {
        return this.value;
    }

    fn getStep() -> int {
        return this.step;
    }

    fn increment() -> int {
        this.value = this.value + this.step;
        return this.value;
    }
}

fn main() -> int {
    let c = alloc Counter(10, 5);

    # Direct member access
    if (c.value != 10) { return 1; }
    if (c.step != 5) { return 2; }

    # Method calls
    if (c.getValue() != 10) { return 3; }
    if (c.getStep() != 5) { return 4; }

    # Method that uses multiple members
    if (c.increment() != 15) { return 5; }
    if (c.getValue() != 15) { return 6; }

    return 50;
}
