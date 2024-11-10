#include <mod_types.h>
#include <debug.h>

extern "C"
{
    std::any module_initialization(int argc, void ** argv)
    {
        log(_log::LOG_NORMAL, "Initializing module...\n");
        log(_log::LOG_NORMAL, "We have ", argc, " argument(s), and the first argument is \"", (const char*)argv[0], "\"\n");
        return 0;
    }

    std::any module_cleanup(int, void ** argv)
    {
        log(_log::LOG_NORMAL, "Cleanup module \"", (const char*)argv[0], "\"...\n");
        return 0;
    }

    std::any example_function(int argc, void ** argv)
    {
        log(_log::LOG_NORMAL, "We have ", argc, " argument(s), and the first argument is \"", (const char*)argv[0], "\"\n");
        for (int i = 0; i < argc; i++) {
            std::cout << "param[" << i << "] = " << *(int*)argv[i+1] << ", ";
        }
        std::cout << std::endl;
        return 1145141919;
    }

    module_description_t __get_module_description__()
    {
        constexpr  module_description_t description {
            .module_name = "ExampleModule"
        };

        return description;
    }
}
