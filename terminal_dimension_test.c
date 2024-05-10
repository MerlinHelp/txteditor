#include <stdlib.h>
#include <stdio.h>

int main(void)
{
    // TESTING popen (opens a process so you can invoke terminal commands)

    FILE *in;
    extern FILE *popen();
    char buf[512];

    if (!(in = popen("tput cols lines", "r"))) {
        exit(1);
    }

    while (fgets(buf, sizeof(buf), in) != NULL) {
        printf("%s", buf);
    }
    
    return EXIT_SUCCESS;

    pclose(in);
} 
