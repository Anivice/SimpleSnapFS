#ifndef MODULE_H
#define MODULE_H

#include <vector>
#include <string>
#include <map>
#include <debug.h>
#include <dlfcn.h>
#include <mod_types.h>

extern class _module_mgr : public graceful_exit_t
{
private:
    std::map<std::string, void*> module_map;

    template < typename Type >
    Type load_symbol(void* handle, const char* symbol)
    {
        dlerror(); // clear existing errors
        Type func = (Type)dlsym(handle, symbol);
        const char* error = dlerror();
        if (error) {
            log(_log::LOG_ERROR, "Failed to load the symbol: ", error, "\n");
            dlclose(handle);
            throw fs_error_t(fs_error_t::SYMBOL_BOT_FOUND_IN_MODULE, *this);
        }
        return func;
    }

    void cleanup();

public:
    template<typename... Args>
    std::string load(const std::string& module_path, Args&&... args)
    {
        void* handle = dlopen(module_path.c_str(), RTLD_LAZY | RTLD_LOCAL);
        if (!handle) {
            log(_log::LOG_ERROR, "Failed to load the library: ", dlerror(), "\n");
            throw fs_error_t(fs_error_t::MODULE_LOADING_FAILED, *this);
        }

        auto get_module_description = load_symbol<get_module_description_t>(handle, "__get_module_description__");
        auto description = get_module_description();
        if (module_map.count(description.module_name)) {
            dlclose(handle);
            throw fs_error_t(fs_error_t::MODULE_EXISTS, *this);
        }

        auto module_initialize = load_symbol<invocation_handler_t>(handle, "module_initialization");
        int arg_count = sizeof...(Args);
        void** arg_array = {reinterpret_cast<void*>(&args)...};
        module_initialize(arg_count, arg_array);

        module_map.emplace(description.module_name, handle);
        return description.module_name;
    }

    void unload(const std::string& module_name);

    template<typename... Args>
    void* call(const std::string& module_name, const std::string& function, Args&&... args)
    {
        auto it = module_map.find(module_name);
        if (it == module_map.end()) {
            throw fs_error_t(fs_error_t::NO_SUCH_MODULE, *this);
        }

        auto func = load_symbol<invocation_handler_t>(it->second, function.c_str());
        int arg_count = sizeof...(Args);
        void** arg_array = {reinterpret_cast<void*>(&args)...};
        return func(arg_count, arg_array);
    }

    ~_module_mgr() { cleanup(); }
} module_mgr;

#endif // MODULE_H
