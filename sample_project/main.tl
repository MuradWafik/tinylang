module main;
import std;
import math;
import geometry;

fn main() -> void {
    std.Println("Testing std generic Println...");
    std.Println(42);
    std.Println(3.14);
    std.Println(true);
    std.PrintErrorLine("Testing std PrintErrorLine...");

    var p1: geometry.Point = geometry.CreatePoint(10, 20);
    p1.MovePoint(5, -5);

    std.Println("Testing direct implicit struct constructor Range...");
    var r1 = Range(1, 4, 1);
    for val in r1 {
        std.PrintInt(val);
    }
    std.PrintChar('\n');

    std.Println("Testing module-prefixed struct constructor std.Range...");
    var r2 = std.Range(4, 7, 1);
    for val in r2 {
        std.PrintInt(val);
    }
    std.PrintChar('\n');

    std.Println("Testing array utilities...");
    var arr = [1, 2, 3, 4, 5, -10, 20];
    var total_sum = std.Sum(arr);
    std.Println(total_sum);

    var x = 42;

    var y = switch(x){
        12 -> "12",
        _ -> "unknown"
    };

    std.Println("y is " + y);
    std.Flush();
}
