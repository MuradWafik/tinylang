native module "std";
native fn print_int(val: int) -> void;
native fn print_char(val: char) -> void;

fn main() -> void {
    var str = "Hello";
    for char_val in str {
        print_char(char_val);
    }
}
