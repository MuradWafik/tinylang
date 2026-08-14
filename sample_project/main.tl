module my_game;
import std;
import math;
import geometry;

fn main() -> void {
    std.Println("Testing std generic Println from my_game module...");
    std.Println(42);
    std.Println(3.14);
    std.Println(true);

    var p1: geometry.Point = geometry.CreatePoint(10, 20);
    p1.MovePoint(5, -5);

    std.Println("Testing array utilities...");
    var arr = [1, 2, 3, 4, 5, -10, 20];
    var total_sum = std.Sum(arr);
    std.Println(total_sum);
    std.Flush();
}
