#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include <string>

namespace data {
class Exception {
public:
    std::string exceptionMessage;
    explicit Exception(std::string exceptionMessage)
        : exceptionMessage(exceptionMessage)
    {
    }
};
}

#endif