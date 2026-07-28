native module "std";

native fn print_int(value: int) -> void;
native fn clock() -> float;

struct Vector2
{
   var x: int;
   var y: int;
}

enum Status {
    Pending,
    Approved = 5,
    Rejected
}

fn (self: Vector2) add(other: Vector2) -> Vector2
{
    var result: Vector2;
    result.x = self.x + other.x;
    result.y = self.y + other.y;
    return result;
}


fn helper(val: int) -> int {
    return val * 3;
}

var arr: int[] = [10, 20, 30, 40];

var i: int = 0;
fn main() -> void
{

while (i < 4) {
    print_int(arr[i]);
    i = i + 1;
}

// 3. Test Array Set
arr[0] = 99;
arr[3] = 77;
// 4. Verify Set worked (should print 99, 20, 30, 77)
i = 0;
while (i < arr.length)
{
    print_int(arr[i]);
    i = i + 1;
}


var vec: Vector2;
vec.x = 20;
vec.y = 10;

var vec2: Vector2;
vec2.x = 5;
vec2.y = 15;

var vec3 = vec.add(vec2);
print_int(vec3.x); // Should print 25
print_int(vec3.y); // Should print 25

var s: Status = Status.Approved;
if (s == Status.Approved) {
    print_int(5); // We can't print 's' directly because it's strictly typed!
}
if (Status.Pending == Status.Pending) {
    print_int(0); 
}
if (Status.Rejected != Status.Approved) {
    print_int(6);
}

}
