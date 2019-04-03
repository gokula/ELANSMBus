//
//  VoodooI2CMultitouchEngine.hpp
//  VoodooI2C
//
//  Created by Alexandre on 22/09/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CMultitouchEngine_hpp
#define VoodooI2CMultitouchEngine_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>

#include "MultitouchHelpers.hpp"

class VoodooI2CMultitouchInterfaceAS;

/* Base class that all mutltitouch engines should inherit from */

class VoodooI2CMultitouchEngineAS : public IOService {
  OSDeclareDefaultStructors(VoodooI2CMultitouchEngineAS);

 public:
    VoodooI2CMultitouchInterfaceAS* interface;

    /* Intended to be overwritten by an inherited class to set the engine's priority
     *
     * @return The engine's score
     */

    virtual UInt8 getScore();

    /* Intended to be overwritten by an inherited class to handle a multitouch event
     * @event The event to be handled
     * @timestamp The event's timestamp
     *
     * @return *MultitouchContinue* if the next engine in line should also be allowed to process the event, *MultitouchBreak* if this is the last engine that should be allowed to process the event
     */

    virtual MultitouchReturn handleInterruptReport(VoodooI2CMultitouchEvent event, AbsoluteTime timestamp);

    /* Sets up the multitouch engine
     * @provider The <VoodooI2CMultitouchInterface> that we have matched against
     *
     * This function is intended to be overwritten by an inherited class but should still be called at the beginning of the overwritten
     * function.
     * @return *true* upon successful start, *false* otherwise
     */

    virtual bool start(IOService* provider);

    /* Stops the multitouch engine
     * @provider The <VoodooI2CMultitouchInterface> that we have matched against
     *
     * This function is intended to be overwritten by an inherited class but should still be called at the end of the the overwritten
     * function.
     */

    virtual void stop(IOService* provider);

 protected:
 private:
};


#endif /* VoodooI2CMultitouchEngine_hpp */
