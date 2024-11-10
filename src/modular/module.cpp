#include <module.h>
#include <debug.h>

void _module_mgr::unload(const std::string & module_name)
{
    auto it = module_map.find(module_name);
    if (it == module_map.end()) {
        // there no override graceful exit for this one, since this is not a class that requires cleanup for every single small error
        throw fs_error_t(fs_error_t::NO_SUCH_MODULE, *this);
    }
}

void _module_mgr::cleanup()
{
    for (const auto & module : module_map) {
        call(module.first, "module_cleanup");
        dlclose(module.second);
    }
}

_module_mgr module_mgr;
