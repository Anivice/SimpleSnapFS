#include <block_io.h>
#include <debug.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>

block_io::block_io(
    const std::string &device_path,
    const uint32_t _block_size)
    :   block_size(_block_size)
{
    fd = open(device_path.c_str(), O_RDWR);
    if (fd == -1)
    {
        log(_log::LOG_ERROR, "Error opening file: ", device_path, "\n");
        throw CannotOpenFile();
    }

    // Use lseek to seek to the end of the file
    auto file_size = lseek(fd, 0, SEEK_END);
    if (file_size == (off64_t)-1)
    {
        log(_log::LOG_ERROR, "Error determining file size: ", device_path, "\n");
        close(fd);
        throw CannotDetermineFileSize();
    }

    total_blocks = file_size / _block_size;
}

block_io::block_t::block_t(
    const uint32_t _block_size,
    const uint64_t _block_number,
    const int fd,
    std::map < uint64_t /* block number */, std::vector < char > > & _cache)
    :   buffer(_block_size),
        cache(_cache),
        block_number(_block_number),
        block_size(_block_size)
{
    auto it = _cache.find(_block_number);

    // not cached, read from disk
    if (it == _cache.end())
    {
        if (lseek(fd, static_cast<off64_t>(_block_number * _block_size), SEEK_SET) == -1)
        {
            log(_log::LOG_ERROR, "Error using lseek\n");
            throw SeekingFailed();
        }

        ssize_t bytes_read = ::read(fd, buffer.data(), _block_size);
        if (bytes_read != _block_size)
        {
            log(_log::LOG_ERROR, "Error reading file\n");
            throw ReadFailed();
        }
    }
    else
    {
        buffer = it->second;
    }
}

uint64_t actual_ops_len(const uint64_t block_size, const uint64_t len, const uint64_t off)
{
    // Ensure the offset is within the block size
    if (off >= block_size) {
        return 0; // Offset is outside the block, no operation possible
    }

    // Calculate the maximum number of bytes that can be processed from the offset to the end of the block
    uint64_t available_length = block_size - off;

    // The actual operation length is the minimum of the requested length and the available length
    return len < available_length ? len : available_length;
}

uint64_t block_io::block_t::read(char * _buf, const uint64_t len, const uint64_t off) const
{
    auto actual_read_len = actual_ops_len(block_size, len, off);
    std::memcpy(_buf, buffer.data(), actual_read_len);
    return actual_read_len;
}

uint64_t block_io::block_t::write(const char * _src, const uint64_t len, const uint64_t off)
{
    auto actual_write_len = actual_ops_len(block_size, len, off);
    std::memcpy(buffer.data(), _src, actual_write_len);
    return actual_write_len;
}

block_io::block_t::~block_t()
{
    cache.insert_or_assign(block_number, buffer);
}

void block_io::sync()
{
    for (const auto& block : cache)
    {
        if (lseek(fd, static_cast<off64_t>(block_size * block.first), SEEK_SET) == -1)
        {
            log(_log::LOG_ERROR, "Error using lseek\n");
            throw SeekingFailed();
        }

        // Write data from the buffer to the file at the specified offset
        ssize_t bytes_written = ::write(fd, block.second.data(), block_size);
        if (bytes_written == -1)
        {
            log(_log::LOG_ERROR, "Error writing to file\n");
            throw WriteFailed();
        }
    }

    cache.clear();
    fsync(fd);
}

block_io::~block_io()
{
    sync();
}

block_io::block_t block_io::get_block(const uint64_t _block_number)
{
    return block_t(block_size, _block_number, fd, cache);
}
