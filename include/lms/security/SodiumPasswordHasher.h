#pragma once

#include "lms/security/IPasswordHasher.h"

namespace lms {
class SodiumPasswordHasher final : public IPasswordHasher {
public:
    SodiumPasswordHasher();
    std::string hash(std::string_view password) const override;
    bool verify(std::string_view password, std::string_view storedHash) const override;
};
} // namespace lms
