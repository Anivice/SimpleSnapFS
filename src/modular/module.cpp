#include <module.h>
#include <debug.h>

void _module_mgr::unload(const std::string &module_name)
{
    auto it = module_map.find(module_name);
    if (it == module_map.end()) {
        log(_log::LOG_ERROR, "Unload failed: No such module named ", module_name, "\n");
        throw fs_error_t(fs_error_t::NO_SUCH_MODULE, *this);
    }

    try {
        call(module_name, "module_cleanup");
    } catch (const fs_error_t& e) {
        log(_log::LOG_ERROR, "Error during module cleanup: ", e.what(), "\n");
    }

    dlclose(it->second);
    module_map.erase(it);
}

void _module_mgr::cleanup()
{
    // Copy keys to a temporary vector to avoid modifying the container while iterating
    std::vector<std::string> keys;
    keys.reserve(module_map.size());
    for (const auto &module : module_map) {
        keys.push_back(module.first);
    }

    for (const auto &key : keys) {
        try {
            call(key, "module_cleanup");
        } catch (const fs_error_t& e) {
            log(_log::LOG_ERROR, "Error during global cleanup for module ", key, ": ", e.what(), "\n");
        }

        dlclose(module_map[key]);
    }

    module_map.clear();
}

_module_mgr module_mgr;
