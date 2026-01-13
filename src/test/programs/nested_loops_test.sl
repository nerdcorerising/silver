# Test nested loops

fn main() -> int {
    # Nested while loops - compute sum of multiplication table
    let sum = 0;
    let i = 1;
    while (i <= 5) {
        let j = 1;
        while (j <= 5) {
            sum = sum + (i * j);
            j = j + 1;
        }
        i = i + 1;
    }

    # Sum should be 225: sum of 5x5 multiplication table
    if (sum != 225) { return 1; }

    # Triple nested loop
    let count = 0;
    let a = 0;
    while (a < 3) {
        let b = 0;
        while (b < 3) {
            let c = 0;
            while (c < 3) {
                count = count + 1;
                c = c + 1;
            }
            b = b + 1;
        }
        a = a + 1;
    }

    if (count != 27) { return 2; }

    return 50;
}
