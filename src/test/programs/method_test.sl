class Counter {
    value: public int;

    fn increment() -> int {
        this.value = this.value + 1;
        return this.value;
    }

    fn add(n: int) -> int {
        this.value = this.value + n;
        return this.value;
    }
}

fn main() -> int {
    let c = alloc Counter(0);
    c.increment();
    c.add(49);
    return c.value;
}
