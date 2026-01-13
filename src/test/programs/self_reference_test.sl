# Test class methods calling other methods on same object

class Calculator {
    result: public int;

    fn reset() {
        this.result = 0;
    }

    fn add(n: int) {
        this.result = this.result + n;
    }

    fn double() {
        this.result = this.result * 2;
    }

    fn compute(a: int, b: int) -> int {
        this.reset();
        this.add(a);
        this.add(b);
        this.double();
        return this.result;
    }
}

fn main() -> int {
    let calc = alloc Calculator(0);
    let result = calc.compute(10, 15);

    if (result != 50) { return 1; }

    return 50;
}
