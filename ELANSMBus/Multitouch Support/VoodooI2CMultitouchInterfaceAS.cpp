//
//  VoodooI2CMultitouchInterface.cpp
//  VoodooI2C
//
//  Created by Alexandre on 22/09/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CMultitouchInterfaceAS.hpp"
#include "VoodooI2CMultitouchEngineAS.hpp"

#define super IOService
OSDefineMetaClassAndStructors(VoodooI2CMultitouchInterfaceAS, IOService);

void VoodooI2CMultitouchInterfaceAS::handleInterruptReport(VoodooI2CMultitouchEvent event, AbsoluteTime timestamp) {
    int i, count;
    VoodooI2CMultitouchEngineAS* engine;

    for (i = 0, count = engines->getCount(); i < count; i++) {
        engine = OSDynamicCast(VoodooI2CMultitouchEngineAS, engines->getObject(i));
        if (!engine)
            continue;

        if (engine->handleInterruptReport(event, timestamp) == MultitouchReturnBreak)
            break;
    }
}

bool VoodooI2CMultitouchInterfaceAS::open(IOService* client) {
    VoodooI2CMultitouchEngineAS* engine = OSDynamicCast(VoodooI2CMultitouchEngineAS, client);

    if (!engine)
        return false;

    engines->setObject(engine);

    return true;
}

SInt8 VoodooI2CMultitouchInterfaceAS::orderEngines(VoodooI2CMultitouchEngineAS* a, VoodooI2CMultitouchEngineAS* b) {
    if (a->getScore() > b->getScore())
        return 1;
    else if (a->getScore() < b->getScore())
        return -1;
    else
        return 0;
}

bool VoodooI2CMultitouchInterfaceAS::start(IOService* provider) {
    if (!super::start(provider)) {
        return false;
    }

    engines = OSOrderedSet::withCapacity(1, (OSOrderedSet::OSOrderFunction)VoodooI2CMultitouchInterfaceAS::orderEngines);

    OSNumber* number = OSNumber::withNumber("0", 32);
    setProperty(kIOFBTransformKey, number);
    setProperty("VoodooI2CServices Supported", OSBoolean::withBoolean(true));

    return true;
}

void VoodooI2CMultitouchInterfaceAS::stop(IOService* provider) {
    if (engines) {
        engines->flushCollection();
        OSSafeReleaseNULL(engines);
    }
}
