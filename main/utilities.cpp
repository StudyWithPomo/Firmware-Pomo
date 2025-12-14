#include "utilities.h"
#include <ctime>

const char *format_time(long epochSeconds, const char *format) {
    static char buffer[64];
    std::time_t t = static_cast<std::time_t>(epochSeconds);
    std::tm *tm_info = std::localtime(&t);
    std::strftime(buffer, sizeof(buffer), format, tm_info);
    return buffer;
}