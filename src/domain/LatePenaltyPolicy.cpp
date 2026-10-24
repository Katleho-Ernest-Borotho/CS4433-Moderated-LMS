#include "lms/domain/LatePenaltyPolicy.h"
#include "lms/domain/UtcDateTime.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace lms {
namespace {
int value(std::string_view s, int start, int width)
{
    int result = 0;
    for (int i=0; i<width; ++i) result=result*10+s[static_cast<std::size_t>(start+i)]-'0';
    return result;
}

std::int64_t civilDays(int year, unsigned month, unsigned day)
{
    year-=month<=2;
    const int era=(year>=0?year:year-399)/400;
    const unsigned yoe=static_cast<unsigned>(year-era*400);
    const unsigned doy=(153*(month+(month>2?-3:9))+2)/5+day-1;
    return static_cast<std::int64_t>(era)*146097+
        static_cast<int>(yoe*365+yoe/4-yoe/100+doy)-719468;
}

std::int64_t seconds(std::string_view s)
{
    validateUtcDateTime(s);
    return civilDays(value(s,0,4),static_cast<unsigned>(value(s,5,2)),
                     static_cast<unsigned>(value(s,8,2)))*86400+
        value(s,11,2)*3600+value(s,14,2)*60+value(s,17,2);
}

double cents(double n) { return std::round(n*100.0)/100.0; }
}

PenaltyBreakdown CommencedDayPenaltyPolicy::calculate(
    double earnedMark, double maximumMark, std::string_view submittedAtUtc,
    std::string_view effectiveDeadlineUtc) const
{
    if (!std::isfinite(earnedMark) || !std::isfinite(maximumMark) ||
        earnedMark<0 || maximumMark<=0 || earnedMark>maximumMark)
        throw std::invalid_argument("Earned mark must be between zero and the assignment maximum");
    const auto lateSeconds=seconds(submittedAtUtc)-seconds(effectiveDeadlineUtc);
    if (lateSeconds<=0) return {0,0.0,cents(earnedMark)};
    const int days=lateSeconds<=86400?1:lateSeconds<=172800?2:
        lateSeconds<=259200?3:4;
    // More than three days late: final is zero and the penalty equals the earned mark.
    if (days==4) return {4,cents(earnedMark),0.0};
    // Commenced 24h windows; the stated deduction is a percent of maximum.
    const double penalty=cents(maximumMark*0.10*days);
    return {days,penalty,cents(std::max(0.0,earnedMark-penalty))};
}
} // namespace lms
