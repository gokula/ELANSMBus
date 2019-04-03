//
//  VoodooCSGestureHIDWrapper.cpp
//  VoodooI2C
//
//  Created by Christopher Luu on 10/7/15.
//  Copyright © 2015 Alexandre Daoud. All rights reserved.
//

#include "VoodooCSGestureHIDWrapperAS.h"
#include "VoodooI2CCSGestureEngineAS.hpp"

OSDefineMetaClassAndStructors(VoodooCSGestureHIDWrapperAS, IOHIDDevice)

bool VoodooCSGestureHIDWrapperAS::start(IOService *provider) {
    if (!IOHIDDevice::start(provider))
        return false;
    setProperty("HIDDefaultBehavior", OSString::withCString("Trackpad"));
    return true;
}

IOReturn VoodooCSGestureHIDWrapperAS::setProperties(OSObject *properties) {
    return kIOReturnUnsupported;
}

IOReturn VoodooCSGestureHIDWrapperAS::newReportDescriptor(IOMemoryDescriptor **descriptor) const {    IOBufferMemoryDescriptor *buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, gestureEngine->reportDescriptorLength());

    if (buffer == NULL) return kIOReturnNoResources;
    gestureEngine->write_report_descriptor_to_buffer(buffer);
    *descriptor = buffer;
    return kIOReturnSuccess;
}

IOReturn VoodooCSGestureHIDWrapperAS::setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
    return kIOReturnUnsupported;
}

IOReturn VoodooCSGestureHIDWrapperAS::getReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
    if (reportType == kIOHIDReportTypeOutput){
        gestureEngine->write_report_to_buffer(report);
        return kIOReturnSuccess;
    }
    return kIOReturnUnsupported;
}

OSString* VoodooCSGestureHIDWrapperAS::newManufacturerString() const {
    return OSString::withCString("CSGesture");
}

OSNumber* VoodooCSGestureHIDWrapperAS::newPrimaryUsageNumber() const {
    return OSNumber::withNumber(kHIDUsage_GD_Mouse, 32);
}

OSNumber* VoodooCSGestureHIDWrapperAS::newPrimaryUsagePageNumber() const {
    return OSNumber::withNumber(kHIDPage_GenericDesktop, 32);
}

OSNumber* VoodooCSGestureHIDWrapperAS::newProductIDNumber() const {
    return OSNumber::withNumber(gestureEngine->productID, 32);
}

OSString* VoodooCSGestureHIDWrapperAS::newProductString() const {
    return OSString::withCString("Trackpad");
}

OSString* VoodooCSGestureHIDWrapperAS::newSerialNumberString() const {
    return OSString::withCString("1234");
}

OSString* VoodooCSGestureHIDWrapperAS::newTransportString() const {
    return OSString::withCString("I2C");
}

OSNumber* VoodooCSGestureHIDWrapperAS::newVendorIDNumber() const {
    return OSNumber::withNumber(gestureEngine->vendorID, 16);
}

OSNumber* VoodooCSGestureHIDWrapperAS::newLocationIDNumber() const {
    return OSNumber::withNumber(123, 32);
}
