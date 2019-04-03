/*
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Copyright (c) 1999-2009 Apple Computer, Inc.  All Rights Reserved.
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

#ifndef _IOKIT_HID_IOHIDPRIVATEKEYS_H_
#define _IOKIT_HID_IOHIDPRIVATEKEYS_H_

#include <sys/cdefs.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/hid/IOHIDProperties.h>

__BEGIN_DECLS

/*!
    @defined HID Device Property Keys
    @abstract Keys that represent properties of a paticular device.
    @discussion Keys that represent properties of a paticular element.  Can be added
        to your matching dictionary when refining searches for HID devices.
*/
#define kIOHIDInputReportElementsKey        "InputReportElements"

#define kIOHIDReportModeKey                 "ReportMode"

typedef enum { 
    kIOHIDReportModeTypeNormal      = 0,
    kIOHIDReportModeTypeFiltered
} IOHIDReportModeType;


/*!
    @defined kIOHIDLogLevelKey
    @abstract Level of detailed logging generated by device driver.
    @discussion The values should match level used in syslog and asl.  
*/

#define kIOHIDLogLevelKey                   "LogLevel"

typedef enum {
     kIOHIDLogLevelTypeEmergency = 0,
     kIOHIDLogLevelTypeAlert,
     kIOHIDLogLevelTypeCritical,
     kIOHIDLogLevelTypeError,
     kIOHIDLogLevelTypeWarning,
     kIOHIDLogLevelTypeNotice,
     kIOHIDLogLevelTypeInfo,
     kIOHIDLogLevelTypeDebug,
     kIOHIDLogLevelTypeTrace
} IOHIDLogLevelType;

/*!
    @defined HID Element Dictionary Keys
    @abstract Keys that represent properties of a particular elements.
    @discussion These keys can also be added to a matching dictionary 
        when searching for elements via copyMatchingElements.  
*/
#define kIOHIDElementCollectionCookieKey    "CollectionCookie"

#define kIOHIDAbsoluteAxisBoundsRemovalPercentage   "AbsoluteAxisBoundsRemovalPercentage"

#define kIOHIDEventServiceQueueSize         "QueueSize"
#define kIOHIDAltSenderIdKey                "alt_sender_id"

#define kIOHIDAppleVendorSupported          "AppleVendorSupported"

#define kIOHIDSetButtonPropertiesKey        "SetButtonProperties"
#define kIOHIDSetButtonPriorityKey          "SetButtonPriority"
#define kIOHIDSetButtonDelayKey             "SetButtonDelay"
#define kIOHIDSetButtonLoggingKey           "SetButtonLogging"

#define kIOHIDSupportsGlobeKeyKey           "SupportsGlobeKey"

#define kIOHIDCompatibilityInterface        "HIDCompatibilityInterface"

#define kIOHIDAuthenticatedDeviceKey        "Authenticated"


/*!
 @typedef IOHIDGameControllerType
 @abstract Describes support game controller types.
 @constant kIOHIDGameControllerTypeStandard Standard game controller.
 @constant kIOHIDGameControllerTypeExtended Extended game controller.
 */
enum {
    kIOHIDGameControllerTypeStandard,
    kIOHIDGameControllerTypeExtended
};
typedef uint32_t IOHIDGameControllerType;

#define kIOHIDGameControllerTypeKey "GameControllerType"

#define kIOHIDGameControllerFormFittingKey "GameControllerFormFitting"




__END_DECLS

#endif /* !_IOKIT_HID_IOHIDPRIVATEKEYS_H_ */
