#include <config/libconfig.h++>
#include <iostream>

int main()
{
    try {
        libconfig::Config config;
        config.readFile(CMAKE_SOURCE_DIR "/src/tests/example_config.cfg");
        auto & modules = config.getRoot()["settings"]["module"];
        const int module_count = modules.getLength();

        for (int i = 0; i < module_count; i++)
        {
            auto & module = modules[i];
            std::string module_name, module_path;
            module.lookupValue("module_name", module_name);
            module.lookupValue("module_path", module_path);
            std::cout << "Module: " << module_name << ": " << std::flush;

            auto & arguments = module.lookup("arguments");
            int arg_count = arguments.getLength();
            for (int j = 0; j < arg_count; j++) {
                std::cout << arguments[j].c_str() << " ";
            }
            std::cout << std::endl;

            std::cout << " ==> Module location: " << module_path << std::endl;
        }
    } catch (libconfig::ConfigException & err) {
        std::cout << err.what() << std::endl;
    }

    return 0;
}
