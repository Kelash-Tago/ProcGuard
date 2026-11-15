#include <stdio.h>
#include <unistd.h>

int main(void)
{
    unsigned long count = 1;

    for (;;) {
        printf("Process is running... Count: %lu\n", count++);
        fflush(stdout);
        sleep(1);
    }

    return 0;
}
