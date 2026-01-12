fn main() -> int {
    let a = 1;
    {
        let b = 2;
        {
            let c = 3;
            if (a + b + c != 6) { return 1; }
        }
        if (a + b != 3) { return 2; }
    }
    if (a != 1) { return 3; }

    # Variable shadowing in different scopes
    let x = 10;
    {
        let x = 20;
        if (x != 20) { return 4; }
    }
    if (x != 10) { return 5; }

    return 50;
}
