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
    var fruits = ["apple", "banana", "cherry"];
    var matrix = [[1, 2], [3, 4]];
    var total_sum = std.Sum(arr);
    std.Println($"Array elements: {arr}");
    std.Println($"Fruits list: {fruits}");
    std.Println($"Matrix: {matrix}");

    var dynamic_scores = int[5];
    var i = 0;
    while(i < dynamic_scores.length)
    {
        dynamic_scores[i] = (i + 1) * 100;
        i = i + 1;
    }
    std.Println($"Dynamically populated scores: {dynamic_scores}");

    var buffer: float[3];
    std.Println($"Default float buffer: {buffer}");

    std.Println($"Array sum: {total_sum}, Point: ({p1.x}, {p1.y})");
    std.Flush();
}
