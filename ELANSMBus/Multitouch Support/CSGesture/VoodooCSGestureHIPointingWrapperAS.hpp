//
//  VoodooCSGestureHIPointingWrapper.hpp
//  VoodooI2C
//
//  Created by CoolStar on 9/6/16.
//  Copyright © 2016 CoolStar. All rights reserved.
//

#ifndef VoodooCSGestureHIPointingWrapper_h
#define VoodooCSGestureHIPointingWrapper_h

#include <IOKit/hidsystem/IOHIPointing.h>

class VoodooI2CCSGestureEngineAS;

class VoodooCSGestureHIPointingWrapperAS : public IOHIPointing {
    typedef IOHIPointing super;
    OSDeclareDefaultStructors(VoodooCSGestureHIPointingWrapperAS);
    
private:
    bool horizontalScroll;
    
protected:
    virtual IOItemCount buttonCount() override;
    virtual IOFixed resolution() override;
    
public:
    VoodooI2CCSGestureEngineAS* gesturerec;
    
    virtual bool init() override;
    
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;
    
    virtual UInt32 deviceType() override;
    virtual UInt32 interfaceID() override;
    
    virtual IOReturn setParamProperties(OSDictionary *dict) override;
    
    void updateRelativeMouse(int dx, int dy, int buttons);
    void updateAbsoluteMouse(SInt16 x, SInt16 y, int buttons);
    void updateScroll(short dy, short dx, short dz);
};

#endif /* VoodooCSGestureHIPointingWrapper_h */
