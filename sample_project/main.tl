native module "std";

native fn print_int(value: int) -> int;
native fn clock() -> float;

fn main() -> int {
    print_int(42);
    return 0;
}

main();