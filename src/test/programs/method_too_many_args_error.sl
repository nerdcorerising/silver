# expect-error: expects 1 argument(s) but got 2

class Counter {
    value: public int;

    fn add(n: int) -> int {
        this.value = this.value + n;
        return this.value;
    }
}

fn main() -> int {
    let c = alloc Counter(0);
    c.add(1, 2);
    return 0;
}
