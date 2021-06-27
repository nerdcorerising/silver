
class MyType {
    x: public int;
    y: public int;
    str: public string;
}

fn main() -> int {
    let userDefined = alloc MyType(1, 50, "Hi!");

    if (userDefined.x == 1
        && userDefined.y == 50
        && userDefined.str == "Hi!")
    {
        return 50;
    }

    return -1;
}