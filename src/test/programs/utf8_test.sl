
# Test UTF-8 string support

fn main() -> int {
    # ASCII string - 5 chars, 5 bytes
    let ascii: string = "hello";
    let ascii_chars: int = strlen_utf8(ascii);
    let ascii_bytes: int = string_bytes(ascii);

    if (ascii_chars != 5) {
        return 1;
    }
    if (ascii_bytes != 5) {
        return 2;
    }

    # 2-byte UTF-8: e-acute (U+00E9) = 0xC3 0xA9
    let two_byte: string = "\u00E9";
    let two_byte_chars: int = strlen_utf8(two_byte);
    let two_byte_bytes: int = string_bytes(two_byte);

    if (two_byte_chars != 1) {
        return 3;
    }
    if (two_byte_bytes != 2) {
        return 4;
    }

    # 3-byte UTF-8: Euro sign (U+20AC) = 0xE2 0x82 0xAC
    let three_byte: string = "\u20AC";
    let three_byte_chars: int = strlen_utf8(three_byte);
    let three_byte_bytes: int = string_bytes(three_byte);

    if (three_byte_chars != 1) {
        return 5;
    }
    if (three_byte_bytes != 3) {
        return 6;
    }

    # Mixed string: "cafe" with accented e = 4 chars, 5 bytes
    let mixed: string = "caf\u00E9";
    let mixed_chars: int = strlen_utf8(mixed);
    let mixed_bytes: int = string_bytes(mixed);

    if (mixed_chars != 4) {
        return 7;
    }
    if (mixed_bytes != 5) {
        return 8;
    }

    # 4-byte UTF-8 using \U: emoji (U+1F600 grinning face) = 0xF0 0x9F 0x98 0x80
    let four_byte: string = "\U0001F600";
    let four_byte_chars: int = strlen_utf8(four_byte);
    let four_byte_bytes: int = string_bytes(four_byte);

    if (four_byte_chars != 1) {
        return 9;
    }
    if (four_byte_bytes != 4) {
        return 10;
    }

    # All tests passed
    return 50;
}
