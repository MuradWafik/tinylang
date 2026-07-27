native module "std";

native fn print_int(value: int) -> void;
native fn clock() -> float;

struct Vector2
{
   var x: int;
   var y: int;
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

print_int(vec.x);

}
