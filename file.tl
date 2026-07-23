fn x(a: int, b: int) -> int
{
    while(a < b)
    {
        a = a + 1;
        if(true)
        {
            break;
        }
    }

    return a;
}

var e: int = x(3, 7);