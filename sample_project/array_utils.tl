module array_utils;
import std;

export fn sum(arr: int[]) -> int {
    var total = 0;
    for val in arr {
        total = total + val;
    }
    return total;
}

export fn count_positive(arr: int[]) -> int {
    var count = 0;
    for val in arr {
        if (val > 0) {
            count = count + 1;
        }
    }
    return count;
}
