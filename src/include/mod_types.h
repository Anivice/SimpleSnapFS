#ifndef MOD_TYPES_H
#define MOD_TYPES_H

#include <any>
#include <vector>
#include <tuple>
#include <stdexcept>

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


// Helper function for casting std::vector < void * > into different types
template <typename... Types, std::size_t... Indices>
std::tuple<Types...> cast_void_ptr_vector_impl(const std::vector<void*>& vec, std::index_sequence<Indices...>) {
    return std::make_tuple(static_cast<Types*>(vec[Indices])...);
}

template <typename... Types>
std::tuple<Types...> cast_void_ptr_vector(const std::vector<void*>& vec) {
    if (vec.size() != sizeof...(Types)) {
        throw std::invalid_argument("Vector size does not match the number of types.");
    }

    return cast_void_ptr_vector_impl<Types...>(vec, std::make_index_sequence<sizeof...(Types)>{});
}

// usage example: auto result = cast_void_ptr_vector < const char *, int, double, const char * >(vec);
// *std::get<0>(result) ==> const char *

#endif //MOD_TYPES_H
