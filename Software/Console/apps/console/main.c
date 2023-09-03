#include "Kernel.h"

int main(void)
{    
    //Call the kernel, this should never return
    kernel_main();

    while(1) {};
}
