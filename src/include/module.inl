bool module_name_sanity_check(const std::string& name)
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

std::vector<std::string> split_string(const std::string &s, char delimiter)
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
    auto pair = split_string(member, '.');
    if (pair.size() != 2) {
        throw fs_error_t(fs_error_t::SYMBOL_NOT_FOUND, *this);
    }

    _module_mgr::mcall_temp_object _temp_object(*this, pair[0], pair[1]);

    return _temp_object;
}

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
std::string _module_mgr::load(const std::string& module_path, Args&&... args)
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

    auto module_initialize = load_symbol<invocation_handler_t>(handle, "module_initialization");
    int arg_count = sizeof...(Args);
    void* arg_array[] = { (void*)(description.module_name), reinterpret_cast<void*>(&args)... };
    if (std::any_cast<int>(module_initialize(arg_count, arg_array)) != 0) {
        log(_log::LOG_ERROR, "Module initialization failed!\n");
        throw fs_error_t(fs_error_t::MODULE_LOADING_FAILED, *this);
    }

    module_map.emplace(description.module_name, handle);
    return description.module_name;
}

template<typename... Args>
std::any _module_mgr::call(const std::string& module_name, const std::string& function, Args&&... args)
{
    auto it = module_map.find(module_name);
    if (it == module_map.end()) {
        throw fs_error_t(fs_error_t::NO_SUCH_MODULE, *this);
    }

    auto func = load_symbol<invocation_handler_t>(it->second, function.c_str());
    int arg_count = sizeof...(Args);
    void* arg_array[] = { (void*)(module_name.c_str()), reinterpret_cast<void*>(&args)... };
    return func(arg_count, arg_array);
}
