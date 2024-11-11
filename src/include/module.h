#ifndef MODULE_H
#define MODULE_H

#include <utility>
#include <vector>
#include <string>
#include <map>
#include <debug.h>
#include <mod_types.h>

extern class _module_mgr : public graceful_exit_t
{
private:
    class mcall_temp_object
    {
    private:
        _module_mgr & manager;
        std::string module_name;
        std::string function_name;

        explicit mcall_temp_object(_module_mgr & _manager, std::string _module_name, std::string _function_name) noexcept
        : manager(_manager), module_name(std::move(_module_name)), function_name(std::move(_function_name)) { }

    public:
        template < typename... Args > std::any operator()(Args&&... args) {
            return manager.call(module_name, function_name, std::forward<Args>(args)...);
        }

        ~mcall_temp_object() = default;
        mcall_temp_object & operator=(const mcall_temp_object&) = delete;
        friend _module_mgr;
    };

    std::map<std::string, void*> module_map;

    template < typename Type > Type load_symbol(void* handle, const char* symbol);
    void cleanup();

public:
    std::string load(const std::string& module_path, const std::vector < std::string > & args);
    void unload(const std::string& module_name);
    template<typename... Args> std::any call(const std::string& module_name, const std::string& function, Args&&... args);
    mcall_temp_object mcall(const std::string& member);
    ~_module_mgr() override { cleanup(); }

} module_mgr;

#include "module.inl"

#endif // MODULE_H
