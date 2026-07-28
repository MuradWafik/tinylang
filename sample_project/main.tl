native module "std";
native fn print_int(value: int) -> void;

struct Vector2
{
   var x: int;
   var y: int;
}

fn (self: Vector2) add(other: Vector2) -> Vector2
{
    // Default constructor for result!
    var result = Vector2();
    result.x = self.x + other.x;
    result.y = self.y + other.y;
    return result;
}

fn main() -> void
{
    // Test 1: Full argument constructor
    var vec1 = Vector2(10, 20);
    print_int(vec1.x); // Should print 10
    print_int(vec1.y); // Should print 20

    // Test 2: Default empty constructor
    var vec2 = Vector2();
    print_int(vec2.x); // Should print 0
    print_int(vec2.y); // Should print 0

    // Modifying the default constructed struct
    vec2.x = 5;
    vec2.y = 15;

    // Test 3: Method call with instantiated structs
    var vec3 = vec1.add(vec2);
    print_int(vec3.x); // Should print 15
    print_int(vec3.y); // Should print 35
}
