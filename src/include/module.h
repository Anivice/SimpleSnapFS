#ifndef MODULE_H
#define MODULE_H

#include <vector>
#include <string>
#include <mod_types.h>
#include <map>
#include <debug.h>
#include <dlfcn.h>

extern class _module_mgr : public graceful_exit_t
{
private:
    std::map < std::string, void * > module_map;
    void cleanup();

public:
    template < typename... Args >
    std::string load(const std::string & module_path, Args&&... args)
    {
        void* handle = dlopen(module_path.c_str(), RTLD_LAZY | RTLD_LOCAL);
        if (!handle)
        {
            log(_log::LOG_ERROR, "Failed to load the library: ", dlerror(), "\n");
            // there no override graceful exit for this one, since this is not a class that requires cleanup for every single small error
            throw fs_error_t(fs_error_t::MODULE_LOADING_FAILED, *this);
        }

        // clear existing errors
        dlerror();

        // load __get_module_description__
        auto get_module_description = (get_module_description_t)dlsym(handle, "__get_module_description__");
        const char * dlsym_error = dlerror();
        if (dlsym_error)
        {
            log(_log::LOG_ERROR, "Failed to load the symbol: ", dlsym_error, "\n");
            dlclose(handle);
            throw fs_error_t(fs_error_t::SYMBOL_BOT_FOUND_IN_MODULE, *this);
        }

        // get description
        auto description = get_module_description();
        auto it = module_map.find(description.module_name);
        if (it != module_map.end()) {
            throw fs_error_t(fs_error_t::MODULE_EXISTS, *this);
        }

        // get module_initialization
        auto module_initialize = (invocation_handler_t)dlsym(handle, "module_initialization");
        const char * dlsym_error2 = dlerror();
        if (dlsym_error2)
        {
            log(_log::LOG_ERROR, "Failed to load the symbol: ", dlsym_error2, "\n");
            dlclose(handle);
            throw fs_error_t(fs_error_t::SYMBOL_BOT_FOUND_IN_MODULE, *this);
        }

        // module initialization
        int arg_count = sizeof...(Args);
        void ** arg_array = { &args... };
        module_initialize(arg_count, arg_array);

        module_map.emplace(description.module_name, handle);

        return description.module_name;
    }

    void unload(const std::string & module_name);

    template < typename... Args >
    void * call(const std::string & module_name, const std::string & function, Args&&... args)
    {
        auto it = module_map.find(module_name);
        if (it == module_map.end()) {
            throw fs_error_t(fs_error_t::NO_SUCH_MODULE, *this);
        }

        int arg_count = sizeof...(Args);        // count of arguments
        void ** arg_array = { &args... };      // array of void pointers to the arguments

        auto func = (invocation_handler_t)dlsym(it->second, function.c_str());
        const char * dlsym_error = dlerror();
        if (dlsym_error)
        {
            log(_log::LOG_ERROR, "Failed to load the symbol: ", dlsym_error, "\n");
            throw fs_error_t(fs_error_t::SYMBOL_BOT_FOUND_IN_MODULE, *this);
        }

        return func(arg_count, arg_array);
    }

    ~_module_mgr() { cleanup(); }
} module_mgr;

#endif //MODULE_H
