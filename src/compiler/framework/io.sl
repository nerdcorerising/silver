
# Print functions using the Silver runtime library
fn print(a: string) -> void {
    silver_print_string(a);
}

fn print_int(a: int) -> void {
    silver_print_int(a);
}

fn print_float(a: float) -> void {
    silver_print_float(a);
}
