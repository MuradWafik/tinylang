native module "std";
native fn print_int(value: int) -> void;

interface IterableInt
{
   fn has_next() -> bool;
   fn next() -> int;
}

struct Range
{
    var start: int;
    var end: int;
    var step: int;
}

extend Range : IterableInt;

fn (self: Range) has_next() -> bool
{
    if(self.step < 0){
        return self.start + self.step >= self.end;
    }
    return self.start + self.step <= self.end;
}

fn (self: Range) next() -> int
{
    var val = self.start;
    self.start = self.start + self.step;
    return val;
}

// --- New Extension Block Test ---

interface Printable {
    fn print() -> void;
}

// Define the method on the primitive receiver
fn (self: int) print() -> void {
    print_int(self);
}

// Bolting the interface onto the primitive int!
extend int : Printable;

fn main() -> void {

    for i in [1, 2, 3, 4]
    {
        print_int(i);
    }

    var my_number = 42;
    my_number.print();

}

