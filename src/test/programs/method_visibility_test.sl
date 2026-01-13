# Test that public methods/fields work and private ones can be accessed internally

class Calculator {
    result: private int;

    private fn double(n: int) -> int {
        return n * 2;
    }

    public fn compute(n: int) -> int {
        this.result = this.double(n);
        return this.result;
    }

    fn getValue() -> int {
        return this.result;
    }
}

fn main() -> int {
    let calc = alloc Calculator(0);
    calc.compute(25);
    return calc.getValue();
}
