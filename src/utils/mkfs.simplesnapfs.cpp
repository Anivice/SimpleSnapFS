#include <utility.h>
#include <cstdint> // For uint64_t
#include <cmath>
#include <debug.h>
#include <simplesnapfs.h>
#include <chrono>
#include <checksum.h>
#include <cstring>

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

#define KBYTES(n) (1024 * n)
#define MBYTES(n) (1024 * KBYTES(n))

void block_size_sanity_check(const uint32_t block_size)
{
    const uint32_t available_block_size [] = {
        512, 1024, KBYTES(2), KBYTES(4), KBYTES(8),
        KBYTES(16), KBYTES(32), KBYTES(64), KBYTES(128),
        KBYTES(256), KBYTES(512), MBYTES(1), MBYTES(2),
        MBYTES(4), MBYTES(8), MBYTES(16), MBYTES(32), MBYTES(64)
    };

    for (const auto & size : available_block_size) {
        if (block_size == size) {
            return;
        }
    }

    log(_log::LOG_ERROR, "Invalid block size: ", block_size, "\n"
        "The block size can only be one of the following:\n"
        "   512, 1024, 2048, 4096, 8192,"
        "   16384, 32768, 65536, 131072,"
        "   262144, 524288, 1048576, 2097152,"
        "   4194304, 8388608, 16777216, 33554432, 67108864\n");

    exit(EXIT_FAILURE);
}

int main(int argc, char ** argv)
{
    const option options[] = {
        {"version", no_argument,       nullptr, 'v'},
        {"help",    no_argument,       nullptr, 'h'},
        {"device",  required_argument, nullptr, 'd'},
        {"label",   required_argument, nullptr, 'L'},
        {"block_size", required_argument, nullptr, 'B'},
        {nullptr,   0,                 nullptr,  0 }  // End of options
    };
    auto arguments = parse_arguments(argc, argv, options, "vhd:L:B:");

    // flags:
    std::string device, label;
    unsigned int block_size = 4096;

    for (auto arg = arguments.begin(); arg != arguments.end(); ++arg)
    {
        if (*arg == "-h") {
            // The user query help, ignore all other options
            output_help(argv[0], std::cout);
            return EXIT_SUCCESS;
        } else if (*arg == "-v") {
            // The user query version, ignore all other options
            output_version(std::cout);
            return EXIT_SUCCESS;
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

    if (device.empty()) {
        log(_log::LOG_ERROR, "You have to provide a device path!\n");
        return EXIT_FAILURE;
    }

    block_size_sanity_check(block_size);

    log(_log::LOG_NORMAL, "Proceeding with the following setup:\n");
    log(_log::LOG_NORMAL, "Label:       ", (label.empty() ? "None" : label), "\n");
    log(_log::LOG_NORMAL, "Block size:  ", block_size, "\n");
    log(_log::LOG_NORMAL, "Device path: ", device, "\n");

    // TODO: calculate device size,
    log(_log::LOG_NORMAL, "Calculating filesystem layout...");
    auto head = make_head(4096, (4ULL * 1024 * 1024 * 1024 * 1024) / (4096), "SampleDisk");

    // Output results
    _log::output_to_stream(std::cout, "done.\n");
    log(_log::LOG_NORMAL, "─────────────────────────────────────────────────────────────────────────────────────────────\n");
    log(_log::LOG_NORMAL, "Total Blocks      = ", head.static_information.fs_total_blocks, "\n");
    log(_log::LOG_NORMAL, "Block Size        = ", head.static_information.fs_block_size, "\n");
    log(_log::LOG_NORMAL, "Utilized Blocks   = ", head.static_information.utilized_blocks, "\n");
    log(_log::LOG_NORMAL, "Discarded Blocks  = ", head.static_information.fs_total_blocks - head.static_information.utilized_blocks, "\n");
    log(_log::LOG_NORMAL, "  ┌────┬─ Logic Blocks = ", head.static_information.logic_blocks, "\n");
    log(_log::LOG_NORMAL, "  │    ├────── Data Block Bitmap Blocks = ", head.static_information.data_block_bitmap_blocks, "\n");
    log(_log::LOG_NORMAL, "  │    ├────── Redundancy Data Block Bitmap Blocks = ", head.static_information.redundancy_data_block_bitmap_blocks, "\n");
    log(_log::LOG_NORMAL, "  │    ├────── Data Block Bitmap Checksum Blocks = ", head.static_information.data_block_bitmap_checksum_blocks, "\n");
    log(_log::LOG_NORMAL, "  │    ├────── Redundancy Data Block Bitmap Checksum Blocks = ", head.static_information.redundancy_data_block_bitmap_checksum_blocks, "\n");
    log(_log::LOG_NORMAL, "  │    ├────── Data Blocks = ", head.static_information.data_blocks, "\n");
    log(_log::LOG_NORMAL, "  │    ├────── Data Block Sha512sum Checksum Blocks = ", head.static_information.data_block_checksum_blocks, "\n");
    log(_log::LOG_NORMAL, "  │    └────── Redundancy Data Block Checksum Sha512sum Blocks = ", head.static_information.redundancy_data_block_checksum_blocks, "\n");
    log(_log::LOG_NORMAL, "  └────┬─ Utility Blocks = ", head.static_information.utility_blocks, "\n");
    log(_log::LOG_NORMAL, "       ├────── Filesystem Head Static Backup Blocks = 5", "\n");
    log(_log::LOG_NORMAL, "       └────── Journaling Buffer Blocks = ", head.static_information.journaling_buffer_blocks, "\n");
    log(_log::LOG_NORMAL, "─────────────────────────────────────────────────────────────────────────────────────────────\n");
    log(_log::LOG_NORMAL, "Filesystem layout:\n");
    log(_log::LOG_NORMAL, "  ┌──────────────────────────────────┐ \n");
    log(_log::LOG_NORMAL, "  │          FILESYSTEM HEAD         │ * 1 block\n");
    log(_log::LOG_NORMAL, "  ├──────────────────────────────────┤ \n");
    log(_log::LOG_NORMAL, "  │         DATA BLOCK BITMAP        │ * ", head.static_information.data_block_bitmap_blocks, " block(s)\n");
    log(_log::LOG_NORMAL, "  ├──────────────────────────────────┤ \n");
    log(_log::LOG_NORMAL, "  │         BITMAP REDUNDANCY        │ * ", head.static_information.redundancy_data_block_bitmap_blocks, " block(s)\n");
    log(_log::LOG_NORMAL, "  ├──────────────────────────────────┤ \n");
    log(_log::LOG_NORMAL, "  │          BITMAP CHECKSUM         │ * ", head.static_information.data_block_bitmap_checksum_blocks, " block(s)\n");
    log(_log::LOG_NORMAL, "  ├──────────────────────────────────┤ \n");
    log(_log::LOG_NORMAL, "  │    BITMAP CHECKSUM REDUNDANCY    │ * ", head.static_information.redundancy_data_block_bitmap_checksum_blocks, " block(s)\n");
    log(_log::LOG_NORMAL, "  ├──────────────────────────────────┤ \n");
    log(_log::LOG_NORMAL, "  │             DATA BLOCK           │ * ", head.static_information.data_blocks, " block(s)\n");
    log(_log::LOG_NORMAL, "  ├──────────────────────────────────┤ \n");
    log(_log::LOG_NORMAL, "  │        DATA BLOCK CHECKSUM       │ * ", head.static_information.data_block_checksum_blocks, " block(s)\n");
    log(_log::LOG_NORMAL, "  ├──────────────────────────────────┤ \n");
    log(_log::LOG_NORMAL, "  │  DATA BLOCK CHECKSUM REDUNDANCY  │ * ", head.static_information.redundancy_data_block_checksum_blocks, " block(s)\n");
    log(_log::LOG_NORMAL, "  ├──────────────────────────────────┤ \n");
    log(_log::LOG_NORMAL, "  │         JOURNALING BUFFER        │ * ", head.static_information.journaling_buffer_blocks, " block(s)\n");
    log(_log::LOG_NORMAL, "  ├──────────────────────────────────┤ \n");
    log(_log::LOG_NORMAL, "  │   FILESYSTEM STATIC DATA BACKUP  │ * 1 block\n");
    log(_log::LOG_NORMAL, "  ├──────────────────────────────────┤ \n");
    log(_log::LOG_NORMAL, "  │  FILESYSTEM STATIC DATA CHECKSUM │ * 1 block\n");
    log(_log::LOG_NORMAL, "  ├──────────────────────────────────┤ \n");
    log(_log::LOG_NORMAL, "  │   FILESYSTEM DYNAMIC DATA BACKUP │ * 1 block\n");
    log(_log::LOG_NORMAL, "  ├──────────────────────────────────┤ \n");
    log(_log::LOG_NORMAL, "  │ FILESYSTEM DYNAMIC DATA CHECKSUM │ * 1 block\n");
    log(_log::LOG_NORMAL, "  └──────────────────────────────────┘ \n");

    log(_log::LOG_NORMAL, "Generating checksum...");
    auto static_checksum = sha512sum((const char*)&head.static_information, sizeof(head.static_information));
    auto dynamic_checksum = sha512sum((const char*)&head.dynamic_information, sizeof(head.dynamic_information));
    std::memcpy(head.checksum_filed.static_information_checksum, static_checksum.data(), 64);
    std::memcpy(head.checksum_filed.dynamic_information_checksum, dynamic_checksum.data(), 64);
    _log::output_to_stream(std::cout, "done.\n");

    return 0;
}
