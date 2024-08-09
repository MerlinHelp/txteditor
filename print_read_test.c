#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

int main(void)
{
    int nread;
    int c;
    while (1) {
        while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
            if (nread == -1 && errno != 0) {
                printf("%s", "Error");
                exit(1);
            }
        }

        printf("%d", c);

    }
}
