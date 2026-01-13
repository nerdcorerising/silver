# Test multiple classes interacting

class Vector2 {
    x: public int;
    y: public int;

    fn length_squared() -> int {
        return this.x * this.x + this.y * this.y;
    }
}

class Rectangle {
    width: public int;
    height: public int;

    fn area() -> int {
        return this.width * this.height;
    }

    fn perimeter() -> int {
        return 2 * (this.width + this.height);
    }
}

class Circle {
    radius: public int;

    fn diameter() -> int {
        return this.radius * 2;
    }
}

fn main() -> int {
    let v = alloc Vector2(3, 4);
    if (v.length_squared() != 25) { return 1; }

    let r = alloc Rectangle(5, 10);
    if (r.area() != 50) { return 2; }
    if (r.perimeter() != 30) { return 3; }

    let c = alloc Circle(7);
    if (c.diameter() != 14) { return 4; }

    return 50;
}
