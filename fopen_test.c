#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    FILE *fp;

    if ((fp = fopen(*(argv + 1), "r"))) {
        fclose(fp);
        if ((fp = fopen(*(argv + 1), "a"))) {
            fclose(fp);
            printf("Read & Write Perms Exist");
            return EXIT_SUCCESS;
        }
        printf("Read perms exist");
    }
    return EXIT_SUCCESS;
}
