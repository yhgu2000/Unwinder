#include <iostream>

int
main()
{
  for (std::istream::int_type ch; ch = std::cin.get(), std::cin;
       std::cout.put(ch))
    ;
}
