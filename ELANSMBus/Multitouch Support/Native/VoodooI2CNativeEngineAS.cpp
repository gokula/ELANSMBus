//
//  VoodooI2CNativeEngine.cpp
//  VoodooI2C
//
//  Created by Alexandre on 10/02/2018.
//  Copyright © 2018 Alexandre Daoud and Kishor Prins. All rights reserved.
//

#include "VoodooI2CNativeEngineAS.hpp"

#define super VoodooI2CMultitouchEngineAS
OSDefineMetaClassAndStructors(VoodooI2CNativeEngineAS, VoodooI2CMultitouchEngineAS);

bool VoodooI2CNativeEngineAS::attach(IOService* provider) {
    if (!super::attach(provider))
        return false;

    return true;
}

void VoodooI2CNativeEngineAS::detach(IOService* provider) {
    super::detach(provider);
}

bool VoodooI2CNativeEngineAS::init(OSDictionary* properties) {
    if (!super::init(properties))
        return false;

    return true;
}

MultitouchReturn VoodooI2CNativeEngineAS::handleInterruptReport(VoodooI2CMultitouchEvent event, AbsoluteTime timestamp) {
    if (simulator)
        simulator->constructReport(event, timestamp);

    return MultitouchReturnContinue;
}

void VoodooI2CNativeEngineAS::free() {
    super::free();
}

bool VoodooI2CNativeEngineAS::start(IOService* provider) {
    if (!super::start(provider))
        return false;
    
    parent = provider;
    simulator = OSTypeAlloc(VoodooI2CMT2SimulatorDeviceAS);
    actuator = OSTypeAlloc(VoodooI2CMT2ActuatorDevice);
    
    if (!simulator->init(NULL) ||
        !simulator->attach(this) ||
        !simulator->start(this)) {
        IOLog("%s Could not initialise simulator\n", getName());
        OSSafeReleaseNULL(simulator);
    }
    
    if (!actuator->init(NULL) ||
        !actuator->attach(this) ||
        !actuator->start(this)) {
        IOLog("%s Could not initialise actuator\n", getName());
        OSSafeReleaseNULL(actuator);
    }

    if (!simulator || !actuator)
        return false;

    return true;
}

void VoodooI2CNativeEngineAS::stop(IOService* provider) {
    super::stop(provider);
}
