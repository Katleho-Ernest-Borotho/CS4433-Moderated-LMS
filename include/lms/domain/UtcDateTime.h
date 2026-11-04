#pragma once
#include <initializer_list>
#include <cstddef>
#include <stdexcept>
#include <string_view>

namespace lms {
// Store UTC dates consistently so lexicographic order equals chronological order.
// Format: YYYY-MM-DDTHH:MM:SSZ (always UTC, no offsets or fractional seconds).
inline void validateUtcDateTime(std::string_view value)
{
    if (value.size() != 20 || value[4]!='-' || value[7]!='-' || value[10]!='T'
        || value[13]!=':' || value[16]!=':' || value[19]!='Z') {
        throw std::invalid_argument("Use UTC date format YYYY-MM-DDTHH:MM:SSZ");
    }
    for (int i : {0,1,2,3,5,6,8,9,11,12,14,15,17,18}) {
        if (value[static_cast<std::size_t>(i)] < '0' ||
            value[static_cast<std::size_t>(i)] > '9')
            throw std::invalid_argument("UTC date contains invalid digits");
    }
    const auto number=[&value](int offset,int length) {
        int result=0;
        for(int i=0;i<length;++i) result=result*10+(value[static_cast<std::size_t>(offset+i)]-'0');
        return result;
    };
    const int y=number(0,4),m=number(5,2),d=number(8,2);
    const int hh=number(11,2),mm=number(14,2),ss=number(17,2);
    if(y<1 || m<1 || m>12 || hh>23 || mm>59 || ss>59)
        throw std::invalid_argument("UTC date/time values are invalid");
    const bool leap=(y%4==0 && (y%100!=0 || y%400==0));
    const int days[]={31,leap?29:28,31,30,31,30,31,31,30,31,30,31};
    if(d<1 || d>days[m-1]) throw std::invalid_argument("UTC calendar date is invalid");
}
} // namespace lms
