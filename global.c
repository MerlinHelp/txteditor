#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

void die(const char* s)
{
    perror(s);
    exit(1);
}
