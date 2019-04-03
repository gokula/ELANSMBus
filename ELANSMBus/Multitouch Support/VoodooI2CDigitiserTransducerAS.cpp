//
//  VoodooI2CDigitiserTransducer.cpp
//  VoodooI2C
//
//  Created by Alexandre on 13/09/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CDigitiserTransducerAS.hpp"

#define super OSObject
OSDefineMetaClassAndStructors(VoodooI2CDigitiserTransducerAS, OSObject);

bool VoodooI2CDigitiserTransducerAS::serialize(OSSerialize* serializer) {
    OSDictionary* temp_dictionary = OSDictionary::withCapacity(2);

    bool result = false;

    if (temp_dictionary) {
        temp_dictionary->setObject(kIOHIDElementParentCollectionKey, collection);
        temp_dictionary->serialize(serializer);
        temp_dictionary->release();

        result = true;
    }
    
    return result;
}

VoodooI2CDigitiserTransducerAS* VoodooI2CDigitiserTransducerAS::transducer(DigitiserTransducerType transducer_type, IOHIDElement* digitizer_collection) {
    VoodooI2CDigitiserTransducerAS* transducer = NULL;
    
    transducer = new VoodooI2CDigitiserTransducerAS;
    
    if (!transducer)
        goto exit;
    
    if (!transducer->init()) {
        transducer = NULL;
        goto exit;
    }
    
    transducer->type        = transducer_type;
    transducer->collection  = digitizer_collection;
    transducer->in_range    = false;

exit:
    return transducer;
}
