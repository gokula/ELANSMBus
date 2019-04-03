//
//  VoodooI2CMultitouchEngine.cpp
//  VoodooI2C
//
//  Created by Alexandre on 22/09/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CMultitouchEngineAS.hpp"
#include "VoodooI2CMultitouchInterfaceAS.hpp"

#define super IOService
OSDefineMetaClassAndStructors(VoodooI2CMultitouchEngineAS, IOService);

UInt8 VoodooI2CMultitouchEngineAS::getScore() {
    return 0x0;
}

MultitouchReturn VoodooI2CMultitouchEngineAS::handleInterruptReport(VoodooI2CMultitouchEvent event, AbsoluteTime timestamp) {
    if (event.contact_count)
        IOLog("Contact Count: %d\n", event.contact_count);
    
    for (int index = 0, count = event.transducers->getCount(); index < count; index++) {
        VoodooI2CDigitiserTransducerAS* transducer = OSDynamicCast(VoodooI2CDigitiserTransducerAS, event.transducers->getObject(index));
        
        if (!transducer)
            continue;
        
        if (transducer->tip_switch)
            IOLog("Transducer ID: %d, X: %d, Y: %d, Z: %d, Pressure: %d\n", transducer->secondary_id, transducer->coordinates.x.value(), transducer->coordinates.y.value(), transducer->coordinates.z.value(), transducer->tip_pressure.value());
    }

    return MultitouchReturnContinue;
}

bool VoodooI2CMultitouchEngineAS::start(IOService* provider) {
    if (!super::start(provider))
        return false;

    interface = OSDynamicCast(VoodooI2CMultitouchInterfaceAS, provider);

    if (!interface)
        return false;

    interface->open(this);

    setProperty("VoodooI2CServices Supported", OSBoolean::withBoolean(true));

    registerService();

    return true;
}

void VoodooI2CMultitouchEngineAS::stop(IOService* provider) {
    if (interface) {
        interface->close(this);
        OSSafeReleaseNULL(interface);
    }

    super::stop(provider);
}
