#include "lms/security/SodiumPasswordHasher.h"

#include <sodium.h>
#include <array>
#include <stdexcept>
#include <string>

namespace lms {

SodiumPasswordHasher::SodiumPasswordHasher()
{
    if (sodium_init() < 0) {
        throw std::runtime_error("Unable to initialise password hashing");
    }
}

std::string SodiumPasswordHasher::hash(std::string_view password) const
{
    if (password.empty()) {
        throw std::invalid_argument("Password cannot be empty");
    }
    std::array<char, crypto_pwhash_STRBYTES> encoded{};
    if (crypto_pwhash_str(encoded.data(), password.data(),
                          static_cast<unsigned long long>(password.size()),
                          crypto_pwhash_OPSLIMIT_INTERACTIVE,
                          crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        throw std::runtime_error("Unable to hash password");
    }
    return std::string(encoded.data());
}

bool SodiumPasswordHasher::verify(std::string_view password, std::string_view storedHash) const
{
    if (storedHash.empty()) {
        return false;
    }
    const std::string encoded(storedHash);
    return crypto_pwhash_str_verify(encoded.c_str(), password.data(),
                                   static_cast<unsigned long long>(password.size())) == 0;
}

} // namespace lms
