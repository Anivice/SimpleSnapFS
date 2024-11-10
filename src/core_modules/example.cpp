#include <mod_types.h>
#include <debug.h>

extern "C"
{
    void * module_initialization(int argc, void ** argv)
    {
        log(_log::LOG_NORMAL, "Initializing module...\n");
        log(_log::LOG_NORMAL, "We have ", argc, " argument(s), and the first argument is \"", (const char*)argv[0], "\"\n");
        return nullptr;
    }

    void * module_cleanup(int, void ** argv)
    {
        log(_log::LOG_NORMAL, "Cleanup module \"", (const char*)argv[0], "\"...\n");
        return nullptr;
    }

    module_description_t __get_module_description__()
    {
        constexpr  module_description_t description {
            .module_name = "Example Module"
        };

        return description;
    }
}
