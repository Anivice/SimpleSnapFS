#ifndef MOD_TYPES_H
#define MOD_TYPES_H

#include <any>
#include <vector>

typedef std::vector < void * > Args_t;
typedef std::any    (*invocation_handler_t)     (const Args_t &);

struct module_description_t
{
    const char module_name [128] { };
};

typedef module_description_t (*get_module_description_t)();

/*
 * The module MUST provide the function: module_description_t __get_module_description__();
 */

#endif //MOD_TYPES_H
