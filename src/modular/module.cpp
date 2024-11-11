#include <module.h>
#include <debug.h>

static bool module_name_sanity_check(const std::string& name)
{
    if (name.empty()) {
        return false;
    }

    // Check if the first character is a letter or underscore
    if (!std::isalpha(name[0]) && name[0] != '_') {
        return false;
    }

    // Check remaining characters for letters, digits, or underscores
    for (size_t i = 1; i < name.length(); ++i) {
        if (!std::isalnum(name[i]) && name[i] != '_') {
            return false;
        }
    }

    return true;
}

static std::vector < std::string >
split_string(const std::string &s, const char delimiter = '.')
{
    std::vector<std::string> tokens;    // store the split parts here
    std::string token;
    std::istringstream tokenStream(s);  // use istringstream to read from the string

    // extract each part separated by the delimiter
    while (std::getline(tokenStream, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    return tokens;
}

_module_mgr::mcall_temp_object _module_mgr::mcall(const std::string& member)
{
    auto pair = split_string(member);
    if (pair.size() != 2) {
        throw fs_error_t(fs_error_t::SYMBOL_NOT_FOUND, *this);
    }

    mcall_temp_object _temp_object(*this, pair[0], pair[1]);

    return _temp_object;
}

std::string _module_mgr::load(const std::string& module_path, const std::vector < std::string > & args)
{
    void* handle = dlopen(module_path.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        log(_log::LOG_ERROR, "Failed to load the library: ", dlerror(), "\n");
        throw fs_error_t(fs_error_t::MODULE_LOADING_FAILED, *this);
    }

    auto get_module_description = load_symbol<get_module_description_t>(handle, "__get_module_description__");
    auto description = get_module_description();

    if (!module_name_sanity_check(description.module_name)) {
        log(_log::LOG_ERROR, "Invalid module name!\n");
        throw fs_error_t(fs_error_t::MODULE_LOADING_FAILED, *this);
    }

    if (module_map.count(description.module_name)) {
        dlclose(handle);
        throw fs_error_t(fs_error_t::MODULE_EXISTS, *this);
    }

    std::vector < void * > args_vec;
    args_vec.emplace_back((void*)description.module_name);
    for (const auto & i : args) {
        args_vec.emplace_back((void*)i.c_str());
    }

    auto module_initialize = load_symbol<invocation_handler_t>(handle, "module_initialization");
    if (std::any_cast<int>(module_initialize(args_vec)) != 0) {
        log(_log::LOG_ERROR, "Module initialization failed!\n");
        throw fs_error_t(fs_error_t::MODULE_LOADING_FAILED, *this);
    }

    module_map.emplace(description.module_name, handle);
    return description.module_name;
}

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
