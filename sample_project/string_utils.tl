module string_utils;
native import std;

native fn print_char(val: char) -> void;

export fn print(str: String) -> void {
    for char_val in str {
        print_char(char_val);
    }
}

export fn println(str: String) -> void {
    print(str);
    print_char('\n');
}
