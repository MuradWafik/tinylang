module geometry;

import math;

export struct Point {
    var x: int;
    var y: int;
}

export fn create_point(x: int, y: int) -> Point {
    var p = Point(x, y);
    return p;
}

export fn (self: Point) move_point(dx: int, dy: int) -> void
{
    math.add(self.x, dx);
    math.add(self.y, dy);
}
