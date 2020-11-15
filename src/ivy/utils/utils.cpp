#include "utils.h"
#include <ctime>
#include <iomanip>

namespace ivy {

std::string get_date_time_as_string() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::ostringstream os;
    os << std::put_time(&tm, "%F %T");
    return os.str();
}
}
