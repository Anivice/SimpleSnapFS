#include <mod_types.h>
#include <debug.h>

extern "C"
{
    void * module_initialization(int, void **)
    {
        log(_log::LOG_NORMAL, "Initializing module...\n");
        return nullptr;
    }

    void * module_cleanup(int, void **)
    {
        log(_log::LOG_NORMAL, "Cleanup module...\n");
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
