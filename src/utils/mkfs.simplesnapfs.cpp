#include <utility.h>
#include <cstdint> // For uint64_t
#include <cmath>
#include <debug.h>
#include <simplesnapfs.h>
#include <chrono>

#define PACKAGE_VERSION "0.0.1"
#define PACKAGE_FULLNAME "Simple Snapshot Filesystem Formatting Tool"

void output_version(std::ostream & identifier)
{
    _log::output_to_stream(identifier, PACKAGE_FULLNAME, " ", PACKAGE_VERSION, "\n");
}

void output_help(const char * cmdline_name, std::ostream & identifier)
{
    output_version(identifier);
    _log::output_to_stream(identifier, cmdline_name, " [OPTIONS [PARAMETERS]...]\n",
        "   --version,-V    Output version.\n"
        "   --help,-h       Output this help message.\n"
        "   --verbose,-v    Verbose output.\n"
        "   --label,-L  [label]     Device label, optional.\n"
        "   --device,-d [device]    Specify the device to format.\n"
        "   --block_size,-B [block size]    Specify the block size.\n"
        );
}

uint64_t cal_blk(const uint64_t elements, const uint64_t elements_one_block_occupy) {
    return (elements / elements_one_block_occupy) + ((elements % elements_one_block_occupy == 0) ? 0 : 1);
}

uint64_t get_current_unix_timestamp()
{
    // Get current time since epoch in seconds
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch()
           ).count();
}

// Function to calculate all components of logic_blocks
simplesnapfs_filesystem_head_t make_fs_head(const uint64_t total_blocks, const uint16_t block_size, const char * label)
{
    // Start with higher initial guesses to maximize inode and data blocks
    uint64_t inode_blk_count = total_blocks * 0.055880000f;
    uint64_t data_blk_count = total_blocks * 0.894120000f;

    uint64_t logic_blocks, utility_blocks, journaling_blocks;
    uint64_t inode_bitmap_blk_count, data_blk_bitmap_blk_count;
    uint64_t inode_bitmap_blk_512chesum_filed_blk_count, data_block_bitmap_blk_512chesum_filed_blk_count;
    uint64_t inode_512checksum_blk_count, data_blk_512checksum_blk_count;

    // Iteratively adjust to maximize inode_blk_count and data_blk_count while satisfying constraints
    while (true)
    {
        // Calculate inode and data bitmap block counts
        inode_bitmap_blk_count = cal_blk(inode_blk_count, 8 * block_size);
        data_blk_bitmap_blk_count = cal_blk(data_blk_count, 8 * block_size);

        // Calculate checksum blocks for the bitmaps
        inode_bitmap_blk_512chesum_filed_blk_count = cal_blk(inode_bitmap_blk_count, block_size / (512 / 8));
        data_block_bitmap_blk_512chesum_filed_blk_count = cal_blk(data_blk_bitmap_blk_count, block_size / (512 / 8));

        // Calculate checksum blocks for inode and data blocks
        inode_512checksum_blk_count = cal_blk(inode_blk_count, block_size / (512 / 8));
        data_blk_512checksum_blk_count = cal_blk(data_blk_count, block_size / (512 / 8));

        // Calculate total logic blocks as the sum of all components
        logic_blocks = inode_bitmap_blk_count + data_blk_bitmap_blk_count +
                       inode_bitmap_blk_512chesum_filed_blk_count + data_block_bitmap_blk_512chesum_filed_blk_count +
                       inode_blk_count + inode_512checksum_blk_count +
                       data_blk_count + data_blk_512checksum_blk_count;

        // Calculate utility_blocks and check if we can compute journaling_blocks
        utility_blocks = total_blocks - logic_blocks;

        // Check if utility_blocks is sufficient to include the required 5 + journaling_blocks
        if (utility_blocks >= 5) {
            journaling_blocks = utility_blocks - 5;
            break;
        }

        // If not feasible, reduce inode and data block counts to fit within the constraints
        if (inode_blk_count > 1) inode_blk_count--;
        if (data_blk_count > 1) data_blk_count--;
    }

    auto info_level_detector = [](const uint16_t size)->uint32_t {
        if (size == 512) {
            return 0;
        } else if (size >= 1024 && size <= 2048) {
            return 1;
        } else {
            return 2;
        }
    };

    simplesnapfs_filesystem_head_t fs_head = {
        .static_information = {
            .fs_identification_number = FILESYSTEM_MAGIC_NUMBER,
            .fs_block_count = total_blocks,
            .fs_block_size = block_size,
            .inode_bitmap_block_count = inode_bitmap_blk_count,
            .inode_bitmap_checksum_count = inode_bitmap_blk_512chesum_filed_blk_count,
            .inode_configuration_flag = {
                .inode_info_level = info_level_detector(block_size),
                .reserved = 0,
            },
            .inode_count = inode_blk_count,
            .inode_checksum_filed_count = inode_512checksum_blk_count,
            .data_block_bitmap_block_count = data_blk_bitmap_blk_count,
            .data_block_bitmap_checksum_count = data_block_bitmap_blk_512chesum_filed_blk_count,
            .data_block_configuration_flag = { },
            .data_block_count = data_blk_count,
            .data_block_checksum_filed_count = data_blk_512checksum_blk_count,
            .journaling_buffer_block_count = journaling_blocks,
            .fs_creation_unix_timestamp = get_current_unix_timestamp(),
            .fs_label = { },
            .fs_identification_number_redundancy = FILESYSTEM_MAGIC_NUMBER
        },
        .dynamic_information = {}
    };

    strncpy(fs_head.static_information.fs_label, label,
        std::min(sizeof(fs_head.static_information.fs_label), strlen(label))
    );

    return fs_head;
}

int main(int argc, char ** argv)
{
    const option options[] = {
        {"version", no_argument,       nullptr, 'V'},
        {"help",    no_argument,       nullptr, 'h'},
        {"verbose", no_argument,       nullptr, 'v'},
        {"device",  required_argument, nullptr, 'd'},
        {"label",   required_argument, nullptr, 'L'},
        {"block_size", required_argument, nullptr, 'B'},
        {nullptr,   0,                 nullptr,  0 }  // End of options
    };
    auto arguments = parse_arguments(argc, argv, options, "Vhvd:L:B:");

    // flags:
    bool verbose = false;
    std::string device, label;
    unsigned int block_size = 4096;

    for (auto arg = arguments.begin(); arg != arguments.end(); ++arg)
    {
        if (*arg == "-h") {
            // The user query help, ignore all other options
            output_help(argv[0], std::cout);
            return 0;
        } else if (*arg == "-V") {
            // The user query version, ignore all other options
            output_version(std::cout);
            return 0;
        } else if (*arg == "-v") {
            verbose = true;
        } else if (*arg == "-d") {
            arg += 1;
            device = *arg;
        } else if (*arg == "-L") {
            arg += 1;
            label = *arg;
        } else if (*arg == "-B") {
            arg += 1;
            block_size = strtol(arg->c_str(), nullptr, 10);
        } else {
            log(_log::LOG_ERROR, "Unrecognized option: ", *arg, "\n");
            output_help(argv[0], std::cerr);
            return 1;
        }
    }

    log(_log::LOG_NORMAL, "Proceeding with the following setup:\n");
    log(_log::LOG_NORMAL, "Verbose:     ", (verbose ? "True" : "False"), "\n");
    log(_log::LOG_NORMAL, "Label:       ", (label.empty() ? "None" : label), "\n");
    log(_log::LOG_NORMAL, "Block size:  ", block_size, "\n");
    log(_log::LOG_NORMAL, "Device path: ", device, "\n");

    if (device.empty()) {
        log(_log::LOG_ERROR, "You have to provide a device path!\n");
        return 1;
    }

    // TODO: calculate device size, block_size sanity check, sha512sum library
    auto head = make_fs_head(32 * 1024 * 1024 / 4096, 4096, "SampleDisk");

    // Output results
    std::cout << "total_blocks:                     " << head.static_information.fs_block_count                 << std::endl;
    std::cout << "inode_bitmap_blk_count:           " << head.static_information.inode_bitmap_block_count       << std::endl;
    std::cout << "data_blk_bitmap_blk_count:        " << head.static_information.data_block_bitmap_block_count  << std::endl;
    std::cout << "inode_bitmap_blk_512chesum_filed_blk_count:       " << head.static_information.inode_bitmap_checksum_count << std::endl;
    std::cout << "data_block_bitmap_blk_512chesum_filed_blk_count:  " << head.static_information.data_block_bitmap_checksum_count << std::endl;
    std::cout << "inode_blk_count:                  " << head.static_information.inode_count << std::endl;
    std::cout << "inode_512checksum_blk_count:      " << head.static_information.inode_checksum_filed_count << std::endl;
    std::cout << "data_blk_count:                   " << head.static_information.data_block_count << std::endl;
    std::cout << "data_blk_512checksum_blk_count:   " << head.static_information.data_block_checksum_filed_count << std::endl;
    std::cout << "journaling_blocks:                " << head.static_information.journaling_buffer_block_count << std::endl;

    return 0;
}
