
class Point {
    x: public int;
    y: public int;
}

class Box {
    width: public int;
    height: public int;
}

fn main() -> int {
    # Test 1: Basic alloc gives refcount of 1
    let p1 = alloc Point(10, 20);
    if (silver_refcount(p1) != 1) {
        silver_print_string("FAIL: p1 refcount should be 1");
        silver_print_int(silver_refcount(p1));
        return -1;
    }

    # Test 2: Second object also has refcount 1
    let p2 = alloc Point(30, 40);
    if (silver_refcount(p2) != 1) {
        silver_print_string("FAIL: p2 refcount should be 1");
        return -2;
    }

    # Test 3: First object still has refcount 1
    if (silver_refcount(p1) != 1) {
        silver_print_string("FAIL: p1 refcount changed unexpectedly");
        return -3;
    }

    # Test 4: Object in nested scope
    if (p1.x == 10) {
        let inner = alloc Box(100, 200);
        if (silver_refcount(inner) != 1) {
            silver_print_string("FAIL: inner refcount should be 1");
            return -4;
        }

        # Verify inner object works
        if (inner.width != 100) {
            silver_print_string("FAIL: inner.width wrong");
            return -5;
        }

        if (inner.height != 200) {
            silver_print_string("FAIL: inner.height wrong");
            return -6;
        }

        # inner will be released when this scope exits
    }

    # Test 5: Outer objects still valid after inner scope
    if (p1.x != 10) {
        silver_print_string("FAIL: p1.x corrupted");
        return -7;
    }

    if (p1.y != 20) {
        silver_print_string("FAIL: p1.y corrupted");
        return -8;
    }

    if (p2.x != 30) {
        silver_print_string("FAIL: p2.x corrupted");
        return -9;
    }

    if (p2.y != 40) {
        silver_print_string("FAIL: p2.y corrupted");
        return -10;
    }

    # Test 6: Multiple nested scopes
    let counter = 0;
    while (counter < 3) {
        let temp = alloc Point(counter, counter * 2);
        if (silver_refcount(temp) != 1) {
            silver_print_string("FAIL: loop temp refcount wrong");
            return -11;
        }

        if (temp.x != counter) {
            silver_print_string("FAIL: temp.x wrong in loop");
            return -12;
        }

        counter = counter + 1;
        # temp released each iteration
    }

    # All tests passed
    silver_print_string("All refcount tests passed!");
    return 50;
}
