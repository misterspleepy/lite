#include "sys.h"
void panic(const char* msg)
{
    print_str(msg);
    while(1);
}