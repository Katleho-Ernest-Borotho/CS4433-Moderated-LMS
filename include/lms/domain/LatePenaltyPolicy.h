#pragma once
#include <string_view>

namespace lms {
struct PenaltyBreakdown {
    int commencedLateDays = 0;
    double penaltyMark = 0.0;
    double finalMark = 0.0;
};

class ILatePenaltyPolicy {
public:
    virtual ~ILatePenaltyPolicy() = default;
    virtual PenaltyBreakdown calculate(
        double earnedMark, double maximumMark, std::string_view submittedAtUtc,
        std::string_view effectiveDeadlineUtc) const = 0;
};

class CommencedDayPenaltyPolicy final : public ILatePenaltyPolicy {
public:
    PenaltyBreakdown calculate(
        double earnedMark, double maximumMark, std::string_view submittedAtUtc,
        std::string_view effectiveDeadlineUtc) const override;
};
} // namespace lms
