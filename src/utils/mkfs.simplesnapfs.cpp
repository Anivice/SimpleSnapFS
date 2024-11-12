#include <utility.h>
#include <cstdint> // For uint64_t
#include <cmath>
#include <debug.h>
#include <simplesnapfs.h>
#include <chrono>
#include <strstream>

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

class limited_stack
{
private:
    std::array<uint64_t, 32> elements{}; // Fixed-size array to hold the last 32 elements
    size_t size;                       // Tracks the number of elements currently in the stack
    size_t next_index;                 // Index where the next push will be inserted

public:
    explicit limited_stack() : size(0), next_index(0) {}

    // Push function: Adds an element to the stack
    void push(const uint64_t value)
    {
        elements[next_index] = value;     // Insert the element at the current index
        next_index = (next_index + 1) % 32; // Wrap around if we reach the end of the array
        if (size < 32) {
            size++; // Increase size only until we reach 32
        }
    }

    // Search function: Checks if an element is present in the stack
    [[nodiscard]] bool is_present(const uint64_t value) const
    {
        for (size_t i = 0; i < size; i++) {
            if (elements[i] == value) {
                return true; // Element found
            }
        }
        return false; // Element not found
    }
};

// Function to calculate the unknown variables
simplesnapfs_filesystem_head_t make_head(const uint32_t block_size, const uint64_t block_count, const char * label)
{
    const uint64_t allowed_attempts = block_count;

    if (block_count < 74) {
        log(_log::LOG_ERROR, "Provided blocks are too few (", block_count, " blocks)! At least 74 blocks are needed to initialize the filesystem.\n");
        exit(EXIT_FAILURE);
    }

    limited_stack total_utilized_block_history;
    // Estimating journaling_blocks initially
    uint64_t estimated_journaling_blocks = (block_count - 5) / 51; // Basic estimate, adjust as needed
    bool force_stop = false;

    auto fs_inode_info_level = [](const uint16_t _block_size) -> uint32_t {
        if (_block_size == 512) return 0;
        else if (_block_size == 1024 || _block_size == 2048) return 1;
        else return 3;
    };

    for (uint64_t attempt = 0; attempt < allowed_attempts; attempt++)
    {
        // Calculate dependent variables based on the current estimate
        uint64_t data_blocks = 50 * estimated_journaling_blocks;
        uint64_t data_block_bitmap_blocks = cal_blk(data_blocks, 8 * block_size);
        uint64_t data_blk_bitmap_checksum_filed_blocks = cal_blk(data_block_bitmap_blocks, block_size / (512 / 8));
        uint64_t data_blk_checksum_blocks = cal_blk(data_blocks, block_size / (512 / 8));

        uint64_t logic_blocks = 2 * (data_block_bitmap_blocks + data_blk_bitmap_checksum_filed_blocks + data_blk_checksum_blocks) + data_blocks;
        uint64_t utility_blocks = estimated_journaling_blocks + 5;

        if (total_utilized_block_history.is_present(logic_blocks + utility_blocks) &&
            (logic_blocks + utility_blocks < block_count))
        {
            log(_log::LOG_NORMAL, "Optimization has reached its limit. Not all available blocks can be utilized. Force stopping...\n");
            force_stop = true;
        }

        total_utilized_block_history.push(logic_blocks + utility_blocks);

        // Check if we match the block count
        if (force_stop || utility_blocks + logic_blocks == block_count)
        {
            simplesnapfs_filesystem_head_t head =
            {
                .static_information = {
                    .fs_identification_number = FILESYSTEM_MAGIC_NUMBER,

                    .fs_total_blocks = block_count,
                    .fs_block_size = block_size,
                    .logic_blocks = logic_blocks,
                    .utility_blocks = utility_blocks,

                    .utilized_blocks = logic_blocks + utility_blocks,

                    .data_block_bitmap_blocks = data_block_bitmap_blocks,
                    .redundancy_data_block_bitmap_blocks = data_block_bitmap_blocks,
                    .data_block_bitmap_checksum_blocks = data_blk_bitmap_checksum_filed_blocks,
                    .redundancy_data_block_bitmap_checksum_blocks = data_blk_bitmap_checksum_filed_blocks,
                    .data_blocks = data_blocks,
                    .data_block_checksum_blocks = data_blk_checksum_blocks,
                    .redundancy_data_block_checksum_blocks = data_blk_checksum_blocks,

                    .journaling_buffer_blocks = estimated_journaling_blocks,
                    .fs_creation_unix_timestamp = get_current_unix_timestamp(),

                    .inode_configuration_flag = {
                        .inode_info_level = fs_inode_info_level(block_size),
                    },

                    .redundancy_fs_identification_number = FILESYSTEM_MAGIC_NUMBER
                },

                .dynamic_information = { }
            };

            strncpy(head.static_information.fs_label, label,
                    std::min(sizeof(head.static_information.fs_label), strlen(label))
                );

            return head;
        }

        // Adjust journaling_blocks based on whether we're over or under the target
        if (utility_blocks + logic_blocks < block_count) {
            estimated_journaling_blocks++; // Increase to match
        } else {
            estimated_journaling_blocks--; // Decrease to match
        }
    }

    log(_log::LOG_ERROR, "Calculation failed within the allowed attempts.\n");
    exit(EXIT_FAILURE);
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
            return EXIT_SUCCESS;
        } else if (*arg == "-V") {
            // The user query version, ignore all other options
            output_version(std::cout);
            return EXIT_SUCCESS;
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
            return EXIT_FAILURE;
        }
    }

    log(_log::LOG_NORMAL, "Proceeding with the following setup:\n");
    log(_log::LOG_NORMAL, "Verbose:     ", (verbose ? "True" : "False"), "\n");
    log(_log::LOG_NORMAL, "Label:       ", (label.empty() ? "None" : label), "\n");
    log(_log::LOG_NORMAL, "Block size:  ", block_size, "\n");
    log(_log::LOG_NORMAL, "Device path: ", device, "\n");

    if (device.empty()) {
        log(_log::LOG_ERROR, "You have to provide a device path!\n");
        return EXIT_FAILURE;
    }

    // TODO: calculate device size, block_size sanity check, sha512sum library
    log(_log::LOG_NORMAL, "Calculating filesystem layout...\n");
    auto head = make_head(4096, (4ULL * 1024 * 1024 * 1024 * 1024) / (4096), "SampleDisk");

    // Output results
    log(_log::LOG_NORMAL, "Total Blocks      = ", head.static_information.fs_total_blocks, "\n");
    log(_log::LOG_NORMAL, "Block Size        = ", head.static_information.fs_block_size, "\n");
    log(_log::LOG_NORMAL, "Utilized Blocks   = ", head.static_information.utilized_blocks, "\n");
    log(_log::LOG_NORMAL, "Discarded Blocks  = ", head.static_information.fs_total_blocks - head.static_information.utilized_blocks, "\n");
    log(_log::LOG_NORMAL, "  ┌────┬─ Logic Blocks = ", head.static_information.logic_blocks, "\n");
    log(_log::LOG_NORMAL, "  │    ├────── Data Block Bitmap Blocks = ", head.static_information.data_block_bitmap_blocks, "\n");
    log(_log::LOG_NORMAL, "  │    ├────── Redundancy Data Block Bitmap Blocks = ", head.static_information.redundancy_data_block_bitmap_blocks, "\n");
    log(_log::LOG_NORMAL, "  │    ├────── Data Blocks = ", head.static_information.data_blocks, "\n");
    log(_log::LOG_NORMAL, "  │    ├────── Data Block Sha512sum Checksum Blocks = ", head.static_information.data_block_checksum_blocks, "\n");
    log(_log::LOG_NORMAL, "  │    └────── Redundancy Data Block Checksum Sha512sum Blocks = ", head.static_information.redundancy_data_block_checksum_blocks, "\n");
    log(_log::LOG_NORMAL, "  └────┬─ Utility Blocks = ", head.static_information.utility_blocks, "\n");
    log(_log::LOG_NORMAL, "       ├────── Filesystem Head Static Backup Blocks = 5", "\n");
    log(_log::LOG_NORMAL, "       └────── Journaling Buffer Blocks = ", head.static_information.journaling_buffer_blocks, "\n");
    return 0;
}
