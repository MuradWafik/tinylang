fn Main(args: String[]) -> int
{
    var x: int = 42;
    var y: int = 10;
    var name: String = "TinyLang";
    var flag: bool = true;

    if (x > y && flag)
    {
        Print(name);
        Print("x is greater than y");
    }
    else
    {
        Print("x is not greater than y");
    }

    while (x > 0)
    {
        x = x - 1;
    }

    return 0;
}