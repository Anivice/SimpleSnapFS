#include <init.h>

#include "module.h"

int main(int, char ** argv)
{
    module_initialization(argv[1]);
    module_mgr.mcall("ExampleModule.example_function")(1, 2, 3, 4, 114514);
}
