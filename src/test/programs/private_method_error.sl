# expect-error: private method

class Counter {
    value: public int;

    private fn increment() -> int {
        this.value = this.value + 1;
        return this.value;
    }

    fn add(n: int) -> int {
        let i = 0;
        while (i < n) {
            this.increment();
            i = i + 1;
        }
        return this.value;
    }
}

fn main() -> int {
    let c = alloc Counter(0);
    c.increment();
    return 0;
}
