#ifndef BLOCK_IO_H
#define BLOCK_IO_H

#define _FILE_OFFSET_BITS 64
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <debug.h>

class block_io
{
private:
    int fd;
    uint32_t block_size;
    uint64_t total_blocks;
    std::map < uint64_t /* block number */, std::vector < char > > cache;

    class block_t {
    private:
        std::vector < char > buffer;
        std::map < uint64_t /* block number */, std::vector < char > > & cache;
        const uint64_t block_number;
        const uint32_t block_size;

        explicit block_t(uint32_t _block_size, uint64_t _block_number, int fd,
            std::map < uint64_t /* block number */, std::vector < char > > & _cache);

    public:
        uint64_t read(char * _buf, uint64_t len, uint64_t off) const;
        uint64_t write(const char * _src, uint64_t len, uint64_t off);
        ~block_t();

        block_t & operator=(const block_t&) = delete;
        friend block_io;
    };

public:
    explicit block_io(const std::string & device_path, uint32_t block_size);
    [[nodiscard]] uint64_t get_total_blocks() const { return total_blocks; }
    ~block_io();
    void sync();
    block_t get_block(uint64_t /* block number */);
};

#endif //BLOCK_IO_H
