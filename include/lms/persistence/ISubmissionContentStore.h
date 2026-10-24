#pragma once
#include <string>
#include <string_view>

namespace lms {
struct StagedRef {
    std::string token;
    std::string extension;
};
struct StoredRef {
    std::string token;
};
class ISubmissionContentStore {
public:
    virtual ~ISubmissionContentStore() = default;
    virtual StagedRef stage(std::string_view sourcePath) = 0;
    virtual StoredRef commit(const StagedRef& staged) = 0;
    virtual void discard(const StagedRef& staged) noexcept = 0;
    virtual void remove(const StoredRef& stored) noexcept = 0;
    virtual std::string read(const StoredRef& stored) const = 0;
};
} // namespace lms
