#include <init.h>
#include <config/libconfig.h++>
#include <module.h>
#include <vector>

void module_initialization(const std::string & _config)
{
    libconfig::Config config;
    config.readFile(_config);
    auto & modules = config.getRoot()["settings"]["module"];
    const int module_count = modules.getLength();

    for (int i = 0; i < module_count; i++)
    {
        auto & module = modules[i];
        std::string module_path;
        module.lookupValue("module_path", module_path);

        auto & arguments = module.lookup("arguments");
        std::vector < std::string > module_arguments;
        int arg_count = arguments.getLength();
        for (int j = 0; j < arg_count; j++) {
            std::string arg(arguments[j].c_str());
            module_arguments.emplace_back(arg);
        }

        module_mgr.load(module_path, module_arguments);
    }
}
