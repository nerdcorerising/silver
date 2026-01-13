# expect-error: Unknown method

class Counter {
    value: public int;

    fn increment() -> int {
        this.value = this.value + 1;
        return this.value;
    }
}

fn main() -> int {
    let c = alloc Counter(0);
    c.decrement();
    return 0;
}
