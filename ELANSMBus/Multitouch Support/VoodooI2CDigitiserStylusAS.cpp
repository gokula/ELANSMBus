//
//  VoodooI2CDigitiserStylus.cpp
//  VoodooI2C
//
//  Created by Alexandre on 13/09/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CDigitiserStylusAS.hpp"

#define super VoodooI2CDigitiserTransducer
OSDefineMetaClassAndStructors(VoodooI2CDigitiserStylusAS, VoodooI2CDigitiserTransducerAS);

VoodooI2CDigitiserStylusAS* VoodooI2CDigitiserStylusAS::stylus(DigitiserTransducerType transducer_type, IOHIDElement* digitizer_collection) {
    VoodooI2CDigitiserStylusAS* transducer = NULL;
    
    transducer = new VoodooI2CDigitiserStylusAS;
    
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
