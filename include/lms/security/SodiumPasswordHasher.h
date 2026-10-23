#pragma once
#include <string>
#include <string_view>

namespace lms {
class SodiumPasswordHasher {
public:
    SodiumPasswordHasher();
    std::string hash(std::string_view password) const;
    bool verify(std::string_view password, std::string_view storedHash) const;
};
} // namespace lms
