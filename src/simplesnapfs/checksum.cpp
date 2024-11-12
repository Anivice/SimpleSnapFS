#include <checksum.h>
#include <openssl/evp.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>

std::array<char, 64> sha512sum(const char* _data, uint64_t _dt_len)
{
    std::array<char, 64> hash{};  // Array to hold the raw SHA-512 hash

    // Create and initialize a message digest context
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx == nullptr) {
        throw std::runtime_error("Failed to create OpenSSL context");
    }

    // Initialize the context for SHA-512
    if (EVP_DigestInit_ex(ctx, EVP_sha512(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize SHA-512 digest");
    }

    // Update the context with the data
    if (EVP_DigestUpdate(ctx, _data, _dt_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to update SHA-512 digest");
    }

    // Finalize the digest
    unsigned char temp_hash[64];
    unsigned int length = 0;
    if (EVP_DigestFinal_ex(ctx, temp_hash, &length) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize SHA-512 digest");
    }

    // Ensure the length is correct
    if (length != 64) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("SHA-512 digest has incorrect length");
    }

    // Copy the resulting hash to the std::array<char, 64>
    std::memcpy(hash.data(), temp_hash, 64);

    // Clean up
    EVP_MD_CTX_free(ctx);

    return hash;
}
