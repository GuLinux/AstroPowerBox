#ifndef APB_AMBIENT_H
#define APB_AMBIENT_H

#include <logger.h>

namespace APB {

class Ambient {
public:
    Ambient(logging::Logger &logger);
    void setup();
    float temperature() const;
    float humidity() const;
    float dewpoint() const;
private:
    logging::Logger &logger;
};

}

#endif