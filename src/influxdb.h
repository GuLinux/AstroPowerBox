#pragma once
#include <TaskSchedulerDeclarations.h>

namespace APB {
class InfluxDb {
public:
    static InfluxDb &Instance;
    void setup(Scheduler &scheduler);
    void onWiFiConnected();
    void sendData();
    bool available() const { return connected; }
private:
    bool connected = false;
};
}