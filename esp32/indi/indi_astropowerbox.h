#pragma once

#include "libindi/defaultdevice.h"

class AstroPowerBox : public INDI::DefaultDevice
{
public:
    AstroPowerBox();
    virtual ~AstroPowerBox() = default;

    // You must override this method in your class.
    virtual const char *getDefaultName() override;
    virtual bool initProperties() override;
};
