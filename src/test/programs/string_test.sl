fn main() -> int {
    # String equality
    let s1 = "hello";
    let s2 = "hello";
    let s3 = "world";

    if (s1 == s2) { } else { return 1; }
    if (s1 != s3) { } else { return 2; }

    # Escape sequences
    let tab = "a\tb";
    let newline = "a\nb";
    let quote = "say \"hi\"";
    let backslash = "path\\file";

    # String length
    if (strlen_utf8(s1) != 5) { return 3; }
    if (string_bytes(s1) != 5) { return 4; }

    # Empty string
    let empty = "";
    if (strlen_utf8(empty) != 0) { return 5; }

    return 50;
}
