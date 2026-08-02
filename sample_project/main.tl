module main;
native import std;
import math;
import geometry;
import array_utils;
import string_utils;

native fn print_int(val: int) -> void;
native fn print_char(val: char) -> void;

fn main() -> void {
    string_utils.println("Testing geometry module...");

    // Create a point at (10, 20)
    var p1: geometry.Point = geometry.create_point(10, 20);
    
    p1.move_point(5, -5);

    print_int(p1.x);
    print_int(p1.y);

    string_utils.println("Testing array_utils module...");
    
    var arr = [1, 2, 3, 4, 5, -10, 20];
    var total_sum = array_utils.sum(arr);
    
    print_int(total_sum);

    var pos_count = array_utils.count_positive(arr);
    print_int(pos_count);
}
