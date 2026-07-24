native module "std";

native fn print_int(value: int) -> void;
native fn clock() -> float;


fn helper(val: int) -> int {
    return val * 3;
}

fn main() -> void {
    print_int(helper(12));
    return;
}

main();