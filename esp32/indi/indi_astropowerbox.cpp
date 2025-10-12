#include "indi_astropowerbox.h"

#include <libindi/connectionplugins/connectiontcp.h>
#include <memory>

static std::unique_ptr<AstroPowerBox> astroPowerBoxDriver(new AstroPowerBox());

AstroPowerBox::AstroPowerBox()
{
    setVersion(0, 1);
}

const char *AstroPowerBox::getDefaultName()
{
    return "My Custom Driver";
}

bool AstroPowerBox::initProperties() {
    INDI::DefaultDevice::initProperties();
    setDriverInterface(POWER_INTERFACE | WEATHER_INTERFACE);
    addAuxControls();
    return true;
}
