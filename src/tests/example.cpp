#include <mod_types.h>
#include <debug.h>

extern "C"
{
    std::any module_initialization(Args_t argv)
    {
        log(_log::LOG_NORMAL, "Initializing module...\n");
        log(_log::LOG_NORMAL, "We have ", argv.size(), " argument(s), and the first argument is \"", (const char*)argv[0], "\"\n");
        for (int i = 1; i < argv.size(); i++) {
            std::cout << "param[" << i << "] = " << (const char*)(argv[i]) << ", ";
        }
        std::cout << std::endl;
        return 0;
    }

    std::any module_cleanup(Args_t argv)
    {
        log(_log::LOG_NORMAL, "Cleanup module \"", (const char*)argv[0], "\"...\n");
        return 0;
    }

    std::any example_function(Args_t argv)
    {
        log(_log::LOG_NORMAL, "We have ", argv.size(), " argument(s), and the first argument is \"", (const char*)argv[0], "\"\n");
        for (int i = 1; i < argv.size(); i++) {
            std::cout << "param[" << i << "] = " << *(int*)argv[i] << ", ";
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
