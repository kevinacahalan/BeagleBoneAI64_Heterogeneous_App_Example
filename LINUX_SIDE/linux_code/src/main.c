#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


extern int rpmsg_char_simple_main(void); // FIXME, SHOULD NOT BE HERE
int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    printf("Test print from linux\n");

    int count = 10;
    for (int i = 1; i <= count; i++)
    {
        sleep(1);
        printf("print from linux %d/%d\n", i, count);
    }
    
    rpmsg_char_simple_main();

    printf("Linux hit end of main\n");
    return 0;
}
