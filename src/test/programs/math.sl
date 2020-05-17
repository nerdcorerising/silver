
import math;

fn main() -> int {
    let a = 10;
    let b = -1;

    let max = max(a, b);
    if (max != a)
    {
        return -1;
    }

    let min = min(a, b);
    if (min != b)
    {
        return -1;
    }

    return 50;
}
