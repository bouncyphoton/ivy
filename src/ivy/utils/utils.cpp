#include "utils.h"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace ivy {

std::string get_date_time_as_string() {
#ifdef _MSC_VER
    std::time_t t = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &t);

    std::ostringstream os;
    os << std::put_time(&tm, "%F %T");
#else
    std::time_t t = std::time(nullptr);
    std::tm *tm = localtime(&t);

    std::ostringstream os;
    os << std::put_time(tm, "%F %T");
#endif

    return os.str();
}

}
