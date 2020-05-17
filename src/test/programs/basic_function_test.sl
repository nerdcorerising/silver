
fn main() -> int {
    let x: int;
    let y = function(11);
    x = function(-3);
    let z = function(0);
    let p = functionWhile();

    if ((x + y + z + p) == 13)
    {
        return 50;
    }

    return 100;
}

fn function(a: int) -> int {
    if (a > 10) {
        return 5;
    } elif (a < -2) {
        return 1;
    } else {
        return -3;
    }
}

fn functionWhile() -> int {
    let x: int;
    x = 0;

    while (x < 10)
    {
        x = x + 1;
    }

    return x;
}