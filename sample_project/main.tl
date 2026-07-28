native module "std";
native fn print_int(value: int) -> void;

interface IterableInt{
   fn has_next() -> bool;
   fn next() -> int;
}

struct Range : IterableInt
{
    var start: int;
    var end: int;
    var step: int;
}

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

interface Movable {
    fn move_x() -> void;
}

interface Resizable {
    fn resize() -> void;
}

struct Box : Movable, Resizable {
    var x: int;
    var size: int;
}

fn (self: Box) move_x() -> void {
    self.x = self.x + 1;
}

fn (self: Box) resize() -> void {
    self.size = self.size * 2;
}


fn main() -> void {
    var b = Box(0, 10);
    b.move_x();
    b.resize();
    // print_int(b.x);
    // print_int(b.size);


    var x = Range(10, 0, -2);
    while(x.has_next()){
        print_int(x.start);
        x.next();
    }
}
