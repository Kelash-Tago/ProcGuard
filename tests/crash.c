#include <signal.h>
#include <stdio.h>

int main(void)
{
    puts("Crash Test Program Started!");
    puts("Generating SIGSEGV...");
    fflush(stdout);
    raise(SIGSEGV);
    puts("This message should not execute.");
    return 0;
}
