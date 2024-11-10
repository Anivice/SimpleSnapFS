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

    template < typename Type > Type load_symbol(void* handle, const char* symbol);
    void cleanup();

public:
    template<typename... Args> std::string load(const std::string& module_path, Args&&... args);
    void unload(const std::string& module_name);
    template<typename... Args> void* call(const std::string& module_name, const std::string& function, Args&&... args);
    ~_module_mgr() override { cleanup(); }

} module_mgr;

#include "module.ini"

#endif // MODULE_H
