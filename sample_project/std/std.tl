module std;

export interface Printable
{
    fn ToString() -> String;
}

export interface Iterator
{
    fn HasNext() -> bool;
    fn Next() -> any;
}

export struct Range
{
    var current: int;
    var stop: int;
    var step: int;
}


export fn (self: Range) HasNext() -> bool
{
    if (self.step > 0)
    {
        return self.current < self.stop;
    }
    else
    {
        return self.current > self.stop;
    }
}

export fn (self: Range) Next() -> int
{
    var val = self.current;
    self.current = self.current + self.step;
    return val;
}

extend Range: Iterator;

native import std;
native fn IntToString(val: int) -> String;
native fn FloatToString(val: float) -> String;
native fn Print(str: String) -> void;
native fn PrintError(str: String) -> void;
native fn PrintInt(val: int) -> void;
native fn PrintChar(val: char) -> void;
native fn PrintErrorChar(val: char) -> void;
native fn FlushStdout() -> void;

export fn (self: String) ToString() -> String
{
    return self;
}

export fn (self: int) ToString() -> String
{
    return IntToString(self);
}

export fn (self: float) ToString() -> String
{
    return FloatToString(self);
}

export fn (self: bool) ToString() -> String
{
    if(self)
    {
        return "true";
    }
    else
    {
        return "false";
    }
}

export fn Println(str: String) -> void
{
    Print(str);
    PrintChar('\n');
}

export fn PrintErrorLine(str: String) -> void
{
    PrintError(str);
    PrintErrorChar('\n');
}

export fn Flush() -> void
{
    FlushStdout();
}


export fn Sum(arr: int[]) -> int
{
    var total = 0;
    for val in arr {
        total = total + val;
    }
    return total;
}
