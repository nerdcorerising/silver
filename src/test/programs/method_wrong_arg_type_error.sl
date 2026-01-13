# expect-error: expects type int but got string

class Counter {
    value: public int;

    fn add(n: int) -> int {
        this.value = this.value + n;
        return this.value;
    }
}

fn main() -> int {
    let c = alloc Counter(0);
    c.add("hello");
    return 0;
}
