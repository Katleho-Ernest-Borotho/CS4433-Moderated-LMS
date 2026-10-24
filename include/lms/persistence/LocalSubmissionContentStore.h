#pragma once
#include "lms/persistence/ISubmissionContentStore.h"
#include <filesystem>
#include <string>

namespace lms {
class LocalSubmissionContentStore final : public ISubmissionContentStore {
public:
    explicit LocalSubmissionContentStore(std::string rootDirectory);
    StagedRef stage(std::string_view sourcePath) override;
    StoredRef commit(const StagedRef& staged) override;
    void discard(const StagedRef& staged) noexcept override;
    void remove(const StoredRef& stored) noexcept override;
    std::string read(const StoredRef& stored) const override;
private:
    std::filesystem::path root_;
};
} // namespace lms
