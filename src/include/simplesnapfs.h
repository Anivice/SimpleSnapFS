#ifndef SIMPLESNAPFS_H
#define SIMPLESNAPFS_H

#include <cstdint>

#define FILESYSTEM_MAGIC_NUMBER (0x9CDA317F6B000001ULL)

struct simplesnapfs_filesystem_head_t
{
    struct _static_information {
        uint64_t fs_identification_number { };

        uint64_t fs_total_blocks { };
        uint32_t fs_block_size { };
        uint64_t logic_blocks { };
        uint64_t utility_blocks { };
        uint64_t utilized_blocks { };

        // logic blocks and vectors
        uint64_t filesystem_head_blk_index = 0;

        uint64_t data_block_bitmap_blk_index = 1;
        uint64_t data_block_bitmap_blocks { };

        uint64_t redundancy_data_block_bitmap_blk_index { };
        uint64_t redundancy_data_block_bitmap_blocks { };

        uint64_t data_block_bitmap_checksum_blk_index { };
        uint64_t data_block_bitmap_checksum_blocks { };

        uint64_t redundancy_data_block_bitmap_checksum_blk_index { };
        uint64_t redundancy_data_block_bitmap_checksum_blocks { };

        uint64_t data_block_index { };
        uint64_t data_blocks { };

        uint64_t data_block_checksum_blk_index { };
        uint64_t data_block_checksum_blocks { };

        uint64_t redundancy_data_block_checksum_blk_index { };
        uint64_t redundancy_data_block_checksum_blocks { };

        // utility blocks
        uint64_t journaling_buffer_blk_index { };
        uint64_t journaling_buffer_blocks { };

        // vector
        uint64_t fs_static_data_backup_blk_index { };
        uint64_t fs_static_data_backup_checksum_blk_index { };
        uint64_t fs_dynamic_data_backup_blk_index { };
        uint64_t fs_dynamic_data_backup_checksum_blk_index { };

        uint64_t fs_creation_unix_timestamp { };
        char fs_label[32] { 0 };

        struct _inode_configuration_flag {
            uint32_t inode_info_level:2 = 3;
            uint32_t reserved:30 = 0;
        } inode_configuration_flag { };

        uint64_t redundancy_fs_identification_number { };
    } static_information { };

    struct _dynamic_information {
        uint64_t filesystem_last_mount_unix_timestamp;
        struct _fs_dynamically_set_flags {
            uint64_t is_mounted:1;
            uint64_t reserved:63;
        } fs_dynamically_set_flags;
    } dynamic_information { };

    struct _checksum_filed {
        char static_information_checksum    [64];
        char dynamic_information_checksum   [64];
    } checksum_filed { };
};

static_assert(sizeof(simplesnapfs_filesystem_head_t) < 512, "Header size is too big!");

#endif //SIMPLESNAPFS_H
