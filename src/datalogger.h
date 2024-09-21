#pragma once

namespace APB {
class DataLogger {
public:
    DataLogger();
    void setup();
    bool initialised() const;
    static DataLogger &Instance;
private:

};
}
