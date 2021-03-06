#ifndef IVY_UTILS_H
#define IVY_UTILS_H

#include <glm/glm.hpp>
#include <string>

namespace ivy {

/**
 * \brief Get the current date time as a string formatted as "YYYY-MM-DD HH:MM:SS"
 * \return Current date time as string
 */
std::string get_date_time_as_string();

}

/**
 * \brief Get the number of elements in an array
 */
#define COUNTOF(x) (sizeof((x)) / sizeof((x)[0]))

#endif // IVY_UTILS_H
