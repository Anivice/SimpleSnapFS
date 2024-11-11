#include <utility.h>

std::vector<std::string> parse_arguments(int argc, char** argv, const option long_options[], const char* short_options)
{
    std::vector<std::string> arguments;

    // Process options using getopt_long
    int opt;
    while ((opt = getopt_long(argc, argv, short_options, long_options, nullptr)) != -1)
    {
        switch (opt) {
            case 0:  // Long option with no short equivalent
                // You can add logic to handle long options here if needed
                    if (optarg) {
                        arguments.push_back(std::string("--") + long_options[optind - 1].name + "=" + optarg);
                    } else {
                        arguments.push_back("--" + std::string(long_options[optind - 1].name));
                    }
            break;
            case '?':  // Unrecognized option
                break;
            default:
                    // For short options with a matching case
                    arguments.emplace_back("-" + std::string(1, (char)opt));
            if (optarg) {
                arguments.emplace_back(optarg);  // Add the argument for options like `-o <arg>`
            }
            break;
        }
    }

    // Collect any remaining non-option arguments
    for (int i = optind; i < argc; ++i) {
        arguments.emplace_back(argv[i]);
    }

    return arguments;
}
