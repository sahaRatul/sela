#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include <string>

namespace data {
class Exception {
public:
    const std::string& exceptionMessage;
    explicit Exception(const std::string& exceptionMessage)
        : exceptionMessage(exceptionMessage)
    {
    }
};
}

#endif