#include <module.h>
#include <cstring>

int main(int, char **)
{
    try {
        char pname[512] {};
        strcpy(pname, CMAKE_BINARY_DIR);
        strcpy(pname + strlen(CMAKE_BINARY_DIR), "/libmod_example.so");
        std::vector<std::string> args;
        module_mgr.load(pname, args);
        log(_log::LOG_NORMAL, "Return from function call is ",
            std::any_cast<int>(module_mgr.mcall("ExampleModule.example_function")(1, 2, 3, 4, 5)),
            "\n");
    } catch (fs_error_t & err) {
        std::cerr << err.what() << std::endl;
        return 1;
    }
    return 0;
}
