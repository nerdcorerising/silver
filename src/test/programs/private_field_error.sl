# expect-error: private field

class Counter {
    count: private int;

    fn increment() -> int {
        this.count = this.count + 1;
        return this.count;
    }

    fn getValue() -> int {
        return this.count;
    }
}

fn main() -> int {
    let c = alloc Counter(0);
    c.count = 10;
    return 0;
}
