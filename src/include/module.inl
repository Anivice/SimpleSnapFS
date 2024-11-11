#ifndef MODULE_INL
#define MODULE_INL

#include <dlfcn.h>

template < typename Type >
Type _module_mgr::load_symbol(void* handle, const char* symbol)
{
    dlerror(); // clear existing errors
    Type func = (Type)dlsym(handle, symbol);
    const char* error = dlerror();
    if (error) {
        log(_log::LOG_ERROR, "Failed to load the symbol: ", error, "\n");
        dlclose(handle);
        throw fs_error_t(fs_error_t::SYMBOL_NOT_FOUND, *this);
    }
    return func;
}

template<typename... Args>
std::any _module_mgr::call(const std::string& module_name, const std::string& function, Args&&... args)
{
    auto it = module_map.find(module_name);
    if (it == module_map.end()) {
        throw fs_error_t(fs_error_t::NO_SUCH_MODULE, *this);
    }

    auto func = load_symbol<invocation_handler_t>(it->second, function.c_str());
    Args_t args_vector;
    int arg_count = sizeof...(Args) + 1;
    void* arg_array[] = { (void*)(module_name.c_str()), (void*)(&args)... };

    for (int i = 0; i < arg_count; i++) {
        args_vector.emplace_back(arg_array[i]);
    }

    return func(args_vector);
}

#endif // MODULE_INL
