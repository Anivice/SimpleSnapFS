#ifndef MOD_TYPES_H
#define MOD_TYPES_H

#include <any>

typedef std::any    (*invocation_handler_t)     (int argument_length, void ** argument_vector);
typedef int         (*invocation_translator_t)  (const char * func_name);

struct module_description_t
{
    const char module_name [128] { };
};

typedef module_description_t (*get_module_description_t)();

/*
 * The module MUST provide the function: module_description_t __get_module_description__();
 */

#endif //MOD_TYPES_H
