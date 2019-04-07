//
//  Created by AS on 05/02/2019.
//  Copyright © 2019 Antonio Salado. All rights reserved.
//
#include "ELANSMBus.h"
#include "SMBUSElanConstants.h"

using namespace I2COperations;
OSDefineMetaClassAndStructors(I2CDevice, IOService)

bool I2CDevice::init (OSDictionary* dict)
{
    bool res = super::init(dict);
    DbgPrint("init\n");
    
    transducers = NULL;
    fPCIDevice  = NULL;
    MyWorkLoop  = NULL;
    fInterruptSrc = NULL;
    Lock.holder = NULL;
    I2C_Lock = NULL;
    mt_interface=NULL;
    
    transducers = OSArray::withCapacity(ETP_MAX_FINGERS);
    if (!transducers) {
        return false;
    }
    DigitiserTransducerType type = kDigitiserTransducerFinger;
    for (int i = 0; i < ETP_MAX_FINGERS; i++) {
        VoodooI2CDigitiserTransducerAS* transducer = VoodooI2CDigitiserTransducerAS::transducer(type, NULL);
        transducers->setObject(transducer);
    }
    awake = true;
    ready_for_input = false;
    read_in_progress = false;
    readingreport = false;
    
    return res;
}

void I2CDevice::free(void)
{
    IOLog("SMBUS_ELAN resources have been deallocated\n");
    
    if (Lock.holder) {
       IOLockFree(Lock.holder);
    }
    
    if (I2C_Lock) {
       IORWLockFree(I2C_Lock);
    }

    if (fInterruptSrc && MyWorkLoop)
        MyWorkLoop->removeEventSource(fInterruptSrc);
    if (fInterruptSrc) fInterruptSrc->release();
    
    fPCIDevice->close(this);
    fPCIDevice->release();
    
    super::free();
}



bool I2CDevice::publish_multitouch_interface() {
    mt_interface = new VoodooI2CMultitouchInterfaceAS();
    if (!mt_interface) {
        IOLog("ELANSMBUS No memory to allocate VoodooI2CMultitouchInterface instance\n");
        goto multitouch_exit;
    }
    if (!mt_interface->init(NULL)) {
        IOLog("ELANSMBUS Failed to init multitouch interface\n");
        goto multitouch_exit;
    }
    if (!mt_interface->attach(this)) {
        IOLog("ELANSMBUS Failed to attach multitouch interface\n");
        goto multitouch_exit;
    }
    if (!mt_interface->start(this)) {
        IOLog("ELANSMBUS Failed to start multitouch interface\n");
        goto multitouch_exit;
    }
    // Assume we are a touchpad
    mt_interface->setProperty(kIOHIDDisplayIntegratedKey, false);
    // 0x04f3 is Elan's Vendor Id
    mt_interface->setProperty(kIOHIDVendorIDKey, 0x04f3, 32);
    mt_interface->setProperty(kIOHIDProductIDKey, product_id, 32);
    return true;
multitouch_exit:
    unpublish_multitouch_interface();
    return false;
}
void I2CDevice::unpublish_multitouch_interface() {
    if (mt_interface) {
        mt_interface->stop(this);
         DbgPrint("Unpublish MTInterface");
    }
}

bool I2CDevice::init_device() {
    if (!reset_device()) {
       return false;
    }
 
    UInt8 hw_res_x = 1.2;
    UInt8 hw_res_y = 1;
    UInt32 max_report_x = 3052;
    UInt32 max_report_y = 1888;
    pressure_adjustment = 25;
   //ASM HARDCODE change for non T480s touchpads
    
   // retVal= elan_smbus_get_max(&max_report_x,&max_report_y);
   // IOLog("xxxxxxxxx ----->  %d,%d, maxx maxy\n",max_report_x,max_report_y);
   // retVal=elan_smbus_get_resolution(&hw_res_x, &hw_res_y);
   // IOLog("xxxxxxxxx ----->  %d,%d, resx, resy\n",hw_res_x,hw_res_y);
    
    hw_res_x = (hw_res_x * 10 + 790) * 10 / 254;
    hw_res_y = (hw_res_y * 10 + 790) * 10 / 254;
    if (mt_interface) {
        mt_interface->physical_max_x = max_report_x * 10 / hw_res_x;
        mt_interface->physical_max_y = max_report_y * 10 / hw_res_y;
        mt_interface->logical_max_x = max_report_x;
        mt_interface->logical_max_y = max_report_y;
    }
    return true;
}

void I2CDevice::handle_input_threaded() {
    if (!ready_for_input) {
        return;
    }
    command_gate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &I2CDevice::parse_ELAN_report));
    read_in_progress = false;
    thread_terminate(current_thread());

}

IOReturn I2CDevice::parse_ELAN_report() {

    UInt8 reportData[ETP_MAX_REPORT_LEN];
    for (int i = 0; i < ETP_MAX_REPORT_LEN; i++) {
        reportData[i] = 0;
    }
    
    if(int res=elan_smbus_get_report(reportData)!=0){
     IOLog("Error reading the report\n");
        read_in_progress = false;
        return kIOReturnError;
    }
    if (!transducers) {
        read_in_progress = false;
        return kIOReturnBadArgument;
    }
    if (reportData[ETP_REPORT_ID_OFFSET] != ETP_REPORT_ID) {
      IOLog("Invalid report (%d)\n",  reportData[ETP_REPORT_ID_OFFSET]);
        read_in_progress = false;
        return kIOReturnError;
    }
    
    AbsoluteTime timestamp;
    clock_get_uptime(&timestamp);
    uint64_t timestamp_ns;
    absolutetime_to_nanoseconds(timestamp, &timestamp_ns);
    
// Ignore input for specified time after keyboard usage - Uncomment to use
  //  if (timestamp_ns - keytime < maxaftertyping)
  //      return kIOReturnSuccess;
    
    UInt8* finger_data = &reportData[ETP_FINGER_DATA_OFFSET];
    UInt8 tp_info = reportData[ETP_TOUCH_INFO_OFFSET];
    UInt8 numFingers = 0;
    for (int i = 0; i < ETP_MAX_FINGERS; i++) {
        VoodooI2CDigitiserTransducerAS* transducer = OSDynamicCast(VoodooI2CDigitiserTransducerAS,  transducers->getObject(i));
        transducer->type = kDigitiserTransducerFinger;
        if (!transducer) {
            continue;
        }
        bool contactValid = tp_info & (1U << (3 + i));
    
        transducer->is_valid = contactValid;
        if (contactValid) {
            unsigned int posX = ((finger_data[0] & 0xf0) << 4) | finger_data[1];
            unsigned int posY = ((finger_data[0] & 0x0f) << 8) | finger_data[2];
            unsigned int pressure = finger_data[4] + pressure_adjustment;
            unsigned int mk_x = (finger_data[3] & 0x0f);
            unsigned int mk_y = (finger_data[3] >> 4);
            unsigned int area_x = mk_x;
            unsigned int area_y = mk_y;
            if(mt_interface) {
                transducer->logical_max_x = mt_interface->logical_max_x;
                transducer->logical_max_y = mt_interface->logical_max_y;
                posY = transducer->logical_max_y - posY;
                area_x = mk_x * (transducer->logical_max_x - ETP_FWIDTH_REDUCE);
                area_y = mk_y * (transducer->logical_max_y - ETP_FWIDTH_REDUCE);
            }
            unsigned int major = mk_x;
            unsigned int minor = mk_y;
            if(mk_x < mk_y) {
                major = mk_y;
                minor = mk_x;
            }
            if(pressure > ETP_MAX_PRESSURE) {
                pressure = ETP_MAX_PRESSURE;
            }
            transducer->coordinates.x.update(posX, timestamp);
            transducer->coordinates.y.update(posY, timestamp);
            transducer->physical_button.update(tp_info & 0x01, timestamp);
            transducer->tip_switch.update(1, timestamp);
            transducer->id = i;
            transducer->secondary_id = i;
            numFingers += 1;
            finger_data += ETP_FINGER_DATA_LEN;
        } else {
            transducer->id = i;
            transducer->secondary_id = i;
            transducer->coordinates.x.update(transducer->coordinates.x.last.value, timestamp);
            transducer->coordinates.y.update(transducer->coordinates.y.last.value, timestamp);
            transducer->physical_button.update(0, timestamp);
            transducer->tip_switch.update(0, timestamp);
        }
    }
    // create new VoodooI2CMultitouchEvent
    VoodooI2CMultitouchEvent event;
    event.contact_count = numFingers;
    event.transducers = transducers;
    // send the event into the multitouch interface
    if (mt_interface) {
        mt_interface->handleInterruptReport(event, timestamp);
    }
    
    return kIOReturnSuccess;
}

bool I2CDevice::reset_device() {
    IOReturn retVal = kIOReturnSuccess;
    retVal = elan_smbus_reset();
    IOSleep(100);
    retVal= elan_smbus_set_mode(ETP_ENABLE_ABS);
    IOLog("RESET result: %d",retVal);
    return true;
}

void I2CDevice::release_resources() {
    if (command_gate) {
        MyWorkLoop->removeEventSource(command_gate);
        command_gate->release();
        command_gate = NULL;
    }
    if (fInterruptSrc) {
        fInterruptSrc->disable();
        MyWorkLoop->removeEventSource(fInterruptSrc);
        fInterruptSrc->release();
        fInterruptSrc = NULL;
    }
    if (MyWorkLoop) {
        MyWorkLoop->release();
        MyWorkLoop = NULL;
    }
    if (transducers) {
        for (int i = 0; i < transducers->getCount(); i++) {
            OSObject* object = transducers->getObject(i);
            if (object) {
                object->release();
            }
        }
        OSSafeReleaseNULL(transducers);
    }
}

bool I2CDevice::createWorkLoop(void)
{
    MyWorkLoop = IOWorkLoop::workLoop();
    
    return (MyWorkLoop != 0);
}

IOWorkLoop *I2CDevice::getWorkLoop(void) const
{
    return MyWorkLoop;
}

IOService *I2CDevice::probe (IOService* provider, SInt32* score)
{
    DbgPrint("probe\n");
    *score = 5000;
    return this;
}

IOInterruptEventSource *I2CDevice::CreateDeviceInterrupt(IOInterruptEventSource::Action action,
               IOFilterInterruptEventSource::Filter filter, IOService *provider)
{
    return IOFilterInterruptEventSource::filterInterruptEventSource(this, action, filter, provider);
}


bool I2CDevice::interruptFilter(OSObject *owner, IOFilterInterruptEventSource *sender)
{
    I2CDevice *obj = (I2CDevice *) owner;
    //We need to read both the normal SMBUS status & the HOST_NOTIFY bit
    UInt8 iSt = obj->fPCIDevice->ioRead8(SMBSLVSTS(obj->fBase));
    obj->fSt = obj->fPCIDevice->ioRead8(SMBHSTSTS(obj->fBase));
    
    if((iSt & SMBSLVSTS_HST_NTFY_STS)&&((obj->fSt & SMBHSTSTS_HOST_BUSY)==0))
    return true;
    
    if ((obj->fSt & SMBHSTSTS_HOST_BUSY) != 0 || (obj->fSt & (SMBHSTSTS_INTR | SMBHSTSTS_DEV_ERR | SMBHSTSTS_BUS_ERR | SMBHSTSTS_FAILED | SMBHSTSTS_SMBALERT_STS | SMBHSTSTS_BYTE_DONE)) == 0)
        return false;
    
    return true;
}


void I2CDevice::interruptHandler(OSObject *owner, IOInterruptEventSource *src, int count)
{
    I2CDevice *obj = (I2CDevice *) owner;
    UInt8 Bp; size_t len;

    UInt8 iSt = obj->fPCIDevice->ioRead8(SMBSLVSTS(obj->fBase));

    if(iSt & SMBSLVSTS_HST_NTFY_STS){
        if (read_in_progress){
            goto continueinterrupt;
        }

        read_in_progress = true;

        thread_t new_thread;
        kern_return_t ret = kernel_thread_start(OSMemberFunctionCast(thread_continue_t, owner, &I2CDevice::handle_input_threaded), owner, &new_thread);
      
        if (ret != KERN_SUCCESS) {
            read_in_progress = false;
            IOLog(" Thread error while attemping to get input report\n");
        } else {
            thread_deallocate(new_thread);
        }
    }
    
continueinterrupt:
   
    /* Catch <DEVERR,INUSE> when cold start */
    if (obj->I2C_Transfer.op == I2CNoOp)
        return;
    
    if (obj->fSt & (SMBHSTSTS_DEV_ERR | SMBHSTSTS_BUS_ERR | SMBHSTSTS_FAILED)) {
        obj->I2C_Transfer.error_marker = 1;
        goto done;
    }
    
    if (obj->fSt & SMBHSTSTS_INTR) {        //TO BE REMOVED
       if (obj->I2C_Transfer.op == I2CWriteOp)
            goto done;
        len = 1;
        if (len > 0)
            Bp = 0;
    }

done:
    
    IOLockLock(obj->Lock.holder);
    obj->Lock.event = true;
    IOLockWakeup(obj->Lock.holder, &obj->Lock.event, true);
    IOLockUnlock(obj->Lock.holder);
    obj->fPCIDevice->ioWrite8(SMBHSTSTS(obj->fBase), obj->fSt); //Reset status bits
    return;
}

bool I2CDevice::start(IOService *provider)
{
    bool res;
    uint32_t hostc;
 
    res = super::start(provider);

    if (!(fPCIDevice = OSDynamicCast(IOPCIDevice, provider))) {
        IOPrint("Failed to cast provider\n");
        return false;
    }
    
    // Read QuietTimeAfterTyping configuration value (if available) Not used here anyway
   OSNumber* quietTimeAfterTyping = OSDynamicCast(OSNumber, getProperty("QuietTimeAfterTyping"));
    if (quietTimeAfterTyping != NULL)
        maxaftertyping = quietTimeAfterTyping->unsigned64BitValue() * 1000000; // Convert to nanoseconds
    
    fPCIDevice->retain();
    fPCIDevice->open(this);
    
   hostc = fPCIDevice->configRead8(ICH_SMB_HOSTC);
   if ((hostc & ICH_SMB_HOSTC_HSTEN) == 0) {
        IOPrint("SMBus disabled\n");
        return false;
    }
    
    fBase = fPCIDevice->configRead16(ICH_SMB_BASE) & 0xFFFE;
    fPCIDevice->setIOEnable(true);

    if (hostc & ICH_SMB_HOSTC_SMIEN) {
        IOPrint("No PCI IRQ. Poll mode is not implemented. Unloading.\n");
        return false;
    }

    I2C_Transfer.op = I2CNoOp;
    Lock.holder = IOLockAlloc();
    Lock.event = false;
    I2C_Lock = IORWLockAlloc();
   
    //enable HOST_NOTIFY
    int host_notify_enabled=fPCIDevice->ioRead8(SMBSLVCMD(fBase));
    if(host_notify_enabled==0)
        fPCIDevice->ioWrite8(SMBSLVCMD(fBase), SMBSLVCMD_HST_NTFY_INTREN);
    
   //Clean HOST_NOTIFY just in case
    fPCIDevice->ioWrite8(SMBSLVSTS(fBase), SMBSLVSTS_HST_NTFY_STS);
  
    createWorkLoop();
    if (!(MyWorkLoop = (IOWorkLoop *) getWorkLoop()))
        return false;
    
    fInterruptSrc = CreateDeviceInterrupt(&I2CDevice::interruptHandler, &I2CDevice::interruptFilter,provider);
    
    /* Avoid masking interrupts for other devices that are sharing the interrupt line
     * by immediate enabling of the event source */
    command_gate = IOCommandGate::commandGate(this);
    if (!command_gate || (MyWorkLoop->addEventSource(command_gate) != kIOReturnSuccess)) {
        IOLog("Could not open command gate\n");
        release_resources();
        return false;
    }
    
 
    publish_multitouch_interface();
    if (!init_device()) {
        IOLog("%s Failed to init device\n", getName());
        return NULL;
    }
   
    MyWorkLoop->addEventSource(fInterruptSrc);
    fInterruptSrc->enable();
    
    int r= elan_smbus_initialize();
    
    if (r!=0){  //RETRY
        IOSleep(1000);
        elan_smbus_initialize();
    }
    
    IOSleep(50); //Should be removed probably - TEST
    ready_for_input = true;
    
    setProperty("VoodooI2CServices Supported", OSBoolean::withBoolean(true));
    IOLog("%s VoodooI2CELAN has started\n", getName());

        mt_interface->registerService();
    
    IOSleep(50); //Should be removed probably - TEST
    /* Allow matching of drivers which use our class as a provider class */
    this->registerService();

    return res;
}

void I2CDevice::stop(IOService *provider)
{
    DbgPrint("stop\n");
    release_resources();
    unpublish_multitouch_interface();
    super::stop(provider);
}


void I2CDevice::LockI2CBus()
{
    IORWLockWrite(I2C_Lock);
}
void I2CDevice::UnlockI2CBus()
{
    IORWLockUnlock(I2C_Lock);
}
//Original ICHSMBUS function - to be removed
int I2CDevice::I2CExec(I2COp op, UInt16 addr, void *cmdbuf, size_t cmdlen, void *buf, size_t len)
{
    int ret;
    UInt8 status, control;
    AbsoluteTime deadline;
    /* Wait for bus to be idle */
    for (int retries = 100; retries > 0; retries--) {
        status = fPCIDevice->ioRead8(SMBHSTSTS(fBase));
        if ((status & SMBHSTSTS_HOST_BUSY) == 0)
            break;
        IODelay(ICHSMBUS_DELAY);
    }
    if (status & SMBHSTSTS_HOST_BUSY)
        return 1;
    if (/* Limited to 2-byte data */
        cmdlen > 1 || len > 2)
        return 2;
    fPCIDevice->ioWrite8(SMBHSTADD(fBase), ICH_SMB_TXSLVA_ADDR(addr) |
                         (op == I2CReadOp ? ICH_SMB_TXSLVA_READ : 0));
    if (cmdlen > 0)
        fPCIDevice->ioWrite8(SMBHSTCMD(fBase), ((UInt8 *) cmdbuf)[0]);
    
    I2C_Transfer.op = op;
    I2C_Transfer.error_marker = 0;
    if (op == I2CReadOp) {
        I2C_Transfer.buffer = buf;
        I2C_Transfer.length = len;
    } else {
        if (len > 0)
            fPCIDevice->ioWrite8(SMBHSTDAT0(fBase), ((UInt8 *) buf)[0]);
        if (len > 1)
            fPCIDevice->ioWrite8(SMBHSTDAT0(fBase), ((UInt8 *) buf)[1]);
    }
    
    if (!len)
        control = ICH_SMB_HC_CMD_BYTE;
    else if (len == 1)
        control = ICH_SMB_HC_CMD_BDATA;
    else /* len == 2 */
        control = ICH_SMB_HC_CMD_WDATA;
    
    control |= SMBHSTCNT_INTREN | SMBHSTCNT_START;
    fPCIDevice->ioWrite8(SMBHSTCNT(fBase), control);
    
    clock_interval_to_deadline(ICHIIC_TIMEOUT, kSecondScale, (UInt64 *) &deadline);
    IOLockLock(Lock.holder);
    if (Lock.event) {
        Lock.event = false;
        IOLockUnlock(Lock.holder);
        goto done;
    }
    /* Don't block forever */
    ret = IOLockSleepDeadline(Lock.holder, &Lock.event, deadline, THREAD_INTERRUPTIBLE);
    Lock.event = false;
    IOLockUnlock(Lock.holder);
    if (ret != THREAD_AWAKENED)
        return 3;
    
done:
    if (I2C_Transfer.error_marker)
        return 4;
    
    return 0;
}

int I2CDevice::ReadI2CBus(UInt16 addr, void *cmdbuf, size_t cmdlen, void *buf, size_t len)
{
    return I2CExec(I2CReadOp, addr, cmdbuf, cmdlen, buf, len);
}
int I2CDevice::WriteI2CBus(UInt16 addr, void *cmdbuf, size_t cmdlen, void *buf, size_t len)
{
    return I2CExec(I2CWriteOp, addr, cmdbuf, cmdlen, buf, len);
}


/* Make sure the SMBus host is ready to start transmitting.
 Return 0 if it is, -EBUSY if it is not. */
int I2CDevice::i801_check_pre(void)
{
    int status;
    status = fPCIDevice->ioRead8(SMBHSTSTS(fBase));
   
    if (status & SMBHSTSTS_HOST_BUSY) {
        return -EBUSY;
    }
    
    status &= STATUS_FLAGS;
    
    if (status) {
        IOLog("Clearing status flags (%02x)\n",status);
        fPCIDevice->ioWrite8(SMBHSTSTS(fBase),status);
        status = fPCIDevice->ioRead8(SMBHSTSTS(fBase)) & STATUS_FLAGS;
        if (status) {
            IOLog("Failed clearing status flags (%02x)\n",status);
            return -EBUSY;
        }
    }
    
    status = fPCIDevice->ioRead8(SMBHSTSTS(fBase))  & SMBAUXSTS_CRCE;
    if (status) {
        IOLog("Clearing aux status flags (%02x)\n", status);
        fPCIDevice->ioWrite8(SMBAUXSTS(fBase),status);
        status = fPCIDevice->ioRead8(SMBHSTSTS(fBase))  & SMBAUXSTS_CRCE;
            if (status) {
                IOLog("Failed clearing aux status flags (%02x)\n",status);
                return -EBUSY;
            }
    }
    return 0;
}

int I2CDevice::i801_check_post(int stat)
{
    int result = 0;
    int status=stat;
    /*
     * If the SMBus is still busy, we give up
     */
    if ((status < 0)) {
       IOLog("Transaction timeout\n");
        /* try to stop the current command */
        IOLog("Terminating the current operation\n");
        fPCIDevice->ioWrite8(SMBHSTCNT(fBase),fPCIDevice->ioRead8(SMBHSTCNT(fBase)) | SMBHSTCNT_KILL);
        IODelay(10);
        fPCIDevice->ioWrite8(SMBHSTCNT(fBase),fPCIDevice->ioRead8(SMBHSTCNT(fBase)) & (~SMBHSTCNT_KILL) );
        /* Check if it worked */
        status = fPCIDevice->ioRead8(SMBHSTSTS(fBase));
        if ((status & SMBHSTSTS_HOST_BUSY) || !(status & SMBHSTSTS_FAILED))
            fPCIDevice->ioWrite8(SMBHSTSTS(fBase),STATUS_FLAGS);
        return -ETIMEDOUT;
    }
    
    if (status & SMBHSTSTS_FAILED) {
        result = -EIO;
        IOLog("Transaction failed\n");
    }
    
    if (status & SMBHSTSTS_DEV_ERR) {
          if(fPCIDevice->ioRead8(SMBAUXSTS(fBase))  & SMBAUXSTS_CRCE) {
              fPCIDevice->ioWrite8(SMBAUXSTS(fBase),SMBAUXSTS_CRCE);
              result = -EBADMSG;
              IOLog("PEC error\n");
        } else {
                IOLog("SMB Check post No response\n");
                result = -ENXIO;
        }
    }
    if (status & SMBHSTSTS_BUS_ERR) {
            IOLog("SMB Check post lost arbitration\n");
        result = -EAGAIN;
    }
    /* Clear status flags except BYTE_DONE, to be cleared by caller */
     fPCIDevice->ioWrite8(SMBHSTSTS(fBase),status);
    
    return result;
}


SInt32 I2CDevice::i2c_smbus_read_block_data(UInt8 addr, UInt8 flags, UInt8 command,
                              UInt8 *values)
{
    union i2c_smbus_data data;
    int status;
    status = i2c_smbus_xfer(addr, flags, I2C_SMBUS_READ, command, I2C_SMBUS_BLOCK_DATA, &data);
    if (status){
        return status;
    }
    memcpy(values, &data.block[1], data.block[0]);
    return data.block[0];
}

SInt32 I2CDevice::i2c_smbus_write_block_data(UInt8 addr, UInt8 flags, UInt8 command,
                               UInt8 length, const UInt8 *values)
{
    union i2c_smbus_data data;
    if (length > I2C_SMBUS_BLOCK_MAX)
        length = I2C_SMBUS_BLOCK_MAX;
    data.block[0] = length;
    memcpy(&data.block[1], values, length);
    int status= i2c_smbus_xfer(addr,flags,
                          I2C_SMBUS_WRITE, command,
                          I2C_SMBUS_BLOCK_DATA, &data);
    return status;
}

SInt32 I2CDevice::i2c_smbus_read_byte_data(UInt8 addr, UInt8 flags, UInt8 command,
                                            UInt8 *values)
{
    union i2c_smbus_data data;
    int status;
    status = i2c_smbus_xfer(addr, 0,
                            I2C_SMBUS_READ, 0,
                            I2C_SMBUS_BYTE, &data);
    if (status)
        return status;
    values[0]=data.byte;
    return 1;
}

SInt32 I2CDevice::i2c_smbus_write_byte_data(UInt8 addr, UInt8 flags, UInt8 command,
                                           UInt8 *values)
{
    int status;
    status = i2c_smbus_xfer(addr, 0,
                            I2C_SMBUS_WRITE, command,
                            I2C_SMBUS_BYTE, NULL);
    if (status)
        return status;
    return 1;
}

SInt32 I2CDevice::i2c_smbus_xfer( UInt16 addr,
                   unsigned short flags, char read_write,
                   UInt8 command, int protocol, union i2c_smbus_data *data)
{
    SInt32 res;
    res = __i2c_smbus_xfer( addr, flags, read_write, command, protocol, data);
    return res;
}

SInt32 I2CDevice::__i2c_smbus_xfer( UInt16 addr,
                     unsigned short flags, char read_write,
                     UInt8 command, int protocol, union i2c_smbus_data *data)
{
    int tries;
    SInt32 res;
    for (res = 0, tries = 0; tries <= 10; tries++) {
        res = i801_access( addr, flags,
                          read_write, command,
                          protocol, data);
        if (res != -EAGAIN)
            break;
        IOSleep(10);
    }
    return res;
}

SInt32 I2CDevice::i801_access( UInt16 addr,
                       unsigned short flags, char read_write, UInt8 command,
                       int size, union i2c_smbus_data *data)
{
    int hwpec;
    int block = 0;
    SInt32 ret = 0;
    UInt8 xact = 0;
    hwpec = 0;
    
    switch (size) {
        case I2C_SMBUS_BYTE:
            fPCIDevice->ioWrite8(SMBHSTADD(fBase),((addr & 0x7f) << 1) | (read_write & 0x01));
            if (read_write == I2C_SMBUS_WRITE)
                fPCIDevice->ioWrite8(SMBHSTCMD(fBase),command);
            xact = I801_BYTE;
            break;

        case I2C_SMBUS_BLOCK_DATA:
            fPCIDevice->ioWrite8(SMBHSTADD(fBase),((addr & 0x7f) << 1) | (read_write & 0x01));
            fPCIDevice->ioWrite8(SMBHSTCMD(fBase),command);
            block = 1;
            break;
 
        default:
            ret = -EOPNOTSUPP;
            goto out;
    }
    
    if (hwpec)
        fPCIDevice->ioWrite8(SMBAUXCTL(fBase),fPCIDevice->ioRead8(SMBAUXCTL(fBase)) | SMBAUXCTL_CRC);
    else
        fPCIDevice->ioWrite8(SMBAUXCTL(fBase),fPCIDevice->ioRead8(SMBAUXCTL(fBase)) & (~SMBAUXCTL_CRC));
 
    if (block)
        ret = i801_block_transaction(data, read_write, size,hwpec);
    else
        ret = i801_transaction(xact);
    
out:
    return ret;
}

int I2CDevice::i801_block_transaction(
                                  union i2c_smbus_data *data, char read_write,
                                  int command, int hwpec)
{
    int result = 0;
 
    if (read_write == I2C_SMBUS_WRITE
        || command == I2C_SMBUS_I2C_BLOCK_DATA) {

        if (data->block[0] < 1)
            data->block[0] = 1;
        if (data->block[0] > I2C_SMBUS_BLOCK_MAX)
            data->block[0] = I2C_SMBUS_BLOCK_MAX;
    } else {
        data->block[0] = 32;
    }
    
    int error= i801_set_block_buffer_mode();

    if(error==0)
        result = i801_block_transaction_by_block( data,read_write, hwpec);

    return result;
}

int I2CDevice::i801_set_block_buffer_mode()
{
    fPCIDevice->ioWrite8(SMBAUXCTL(fBase),fPCIDevice->ioRead8(SMBAUXCTL(fBase))| SMBAUXCTL_E32B);

    if ((fPCIDevice->ioRead8(SMBAUXCTL(fBase)) & SMBAUXCTL_E32B) == 0)
        return -EIO;
    
    return 0;
}

int I2CDevice::i801_block_transaction_by_block(  union i2c_smbus_data *data,char read_write, int hwpec)
{
    int i, len;
    int status;

    fPCIDevice->ioRead8(SMBHSTCNT(fBase));
    /* Use 32-byte buffer to process this transaction */
    if (read_write == I2C_SMBUS_WRITE) {
        len = data->block[0];
       fPCIDevice->ioWrite8(SMBHSTDAT0(fBase),len);
        for (i = 0; i < len; i++)
        fPCIDevice->ioWrite8(SMBBLKDAT(fBase),data->block[i+1]);
    }

    status = i801_transaction( I801_BLOCK_DATA | (hwpec ? SMBHSTCNT_PEC_EN : 0));

    if (status)
        return status;

    if (read_write == I2C_SMBUS_READ) {
       len = fPCIDevice->ioRead8(SMBHSTDAT0(fBase));
        if (len < 1 || len > I2C_SMBUS_BLOCK_MAX)
            return -EPROTO;
        data->block[0] = len;
        for (i = 0; i < len; i++)
            data->block[i + 1] = fPCIDevice->ioRead8(SMBBLKDAT(fBase));
    }
    return 0;
}

int I2CDevice::i801_transaction( UInt8 xact)
{
    int status=0;
    int ret;
    int result;
    AbsoluteTime deadline;
    result = i801_check_pre();
    if (result < 0)
        return result;
    fPCIDevice->ioWrite8(SMBHSTCNT(fBase), xact | SMBHSTCNT_INTREN | SMBHSTCNT_START);
    clock_interval_to_deadline(ICHIIC_TIMEOUT, kSecondScale, (UInt64 *) &deadline);
    IOLockLock(Lock.holder);
    if (Lock.event) {
        Lock.event = false;
        IOLockUnlock(Lock.holder);
        goto done;
    }
    /* Don't block forever */
    ret = IOLockSleepDeadline(Lock.holder, &Lock.event, deadline, THREAD_INTERRUPTIBLE);
    Lock.event = false;
    IOLockUnlock(Lock.holder);
    if (ret != THREAD_AWAKENED)
        return 1;
    
done:
    if (I2C_Transfer.error_marker)
        return 1;
    
    return i801_check_post(status);
}

int I2CDevice::elan_smbus_initialize(void)
{
    UInt8 check[ETP_SMBUS_HELLOPACKET_LEN] = { 0x55, 0x55, 0x55, 0x55, 0x55 };
    UInt8 values[I2C_SMBUS_BLOCK_MAX] = {0};
    int len;
  
    I2C_Transfer.op = I2CReadOp;
    I2C_Transfer.error_marker=0;
    UInt8 cmd;
    UInt8 s=0;
    UInt8 a=0;
 
    LockI2CBus();
    a=ReadI2CBus(0x15, &(cmd = 0x04), sizeof cmd, &s, sizeof s);
    IOSleep(30);
    len = i2c_smbus_read_block_data(0x15,0,ETP_SMBUS_HELLOPACKET_CMD, values);
    UnlockI2CBus();
 
    if (len != ETP_SMBUS_HELLOPACKET_LEN) {
        IOPrint("Hello packet lenght fail\n");
        IOPrint("LEN: %d\n",len);
       return -1;
    }
    /* compare hello packet */
    if (memcmp(values, check, ETP_SMBUS_HELLOPACKET_LEN)) {
        IOPrint("HELLO KO");
       return -2;
    }else
         IOPrint("HELLO!!!!!!!!!!\n");
    
    I2C_Transfer.op = I2CWriteOp;
    I2C_Transfer.error_marker=0;
    LockI2CBus();
    a = i2c_smbus_write_byte_data(0x15,0, ETP_SMBUS_ENABLE_TP,NULL);
    UnlockI2CBus();
   
    IOPrint("TOUCHPAD ENABLED\n");
    return 0;
}



UInt8 I2CDevice::elan_smbus_get_max(UInt32 *max_x,UInt32 *max_y)
{
    int ret;
    UInt8 val[I2C_SMBUS_BLOCK_MAX] = {0};
    
    I2C_Transfer.op = I2CReadOp;
    I2C_Transfer.error_marker=0;
    LockI2CBus();
    ret = i2c_smbus_read_block_data(0x15,0, ETP_SMBUS_RANGE_CMD, val);

    UnlockI2CBus();
    *max_x = (0x0f & val[0]) << 8 | val[1];
    *max_y = (0xf0 & val[0]) << 4 | val[2];
  
    return kIOReturnSuccess;
}


 UInt8 I2CDevice::elan_smbus_get_num_traces(UInt8 *x_traces,
                                     UInt8 *y_traces)
{
    int ret;
    int error;
    UInt8 val[I2C_SMBUS_BLOCK_MAX] = {0};
    I2C_Transfer.op = I2CReadOp;
    I2C_Transfer.error_marker=0;
    LockI2CBus();
    ret = i2c_smbus_read_block_data(0x15,0, ETP_SMBUS_XY_TRACENUM_CMD, val);
    if (ret != 3) {
        error = ret < 0 ? ret : -EIO;
        return error;
    }
    UnlockI2CBus();
    *x_traces = val[1];
    *y_traces = val[2];
    return 0;
}

UInt8 I2CDevice::elan_smbus_get_resolution(UInt8 *hw_res_x, UInt8 *hw_res_y)
{
    int ret;
    int error;
    UInt8 val[I2C_SMBUS_BLOCK_MAX] = {0};
    I2C_Transfer.op = I2CReadOp;
    I2C_Transfer.error_marker=0;
    LockI2CBus();
    ret = i2c_smbus_read_block_data(0x15,0, ETP_SMBUS_RESOLUTION_CMD, val);
    if (ret != 3) {
        error = ret < 0 ? ret : -EIO;
        return error;
    }
    UnlockI2CBus();
    
    *hw_res_x = val[1] & 0x0F;
    *hw_res_y = (val[1] & 0xF0) >> 4;
    
    return 0;
}

UInt8 I2CDevice::elan_smbus_reset(void)
{
    UInt8 error,cmd,s;
    cmd = 0x2B;
    s=0x2B;
    struct PList {
        UInt8 reg[2];
        UInt8 value;
        SInt8 duty;
    } Pwm[1];
    Pwm[0] =  { 0x2B, 0, 0 };
    
    I2C_Transfer.op = I2CWriteOp;
    I2C_Transfer.error_marker=0;
    LockI2CBus();
    error = i2c_smbus_write_byte_data(0x15,0, ETP_SMBUS_IAP_RESET_CMD,NULL);
    UnlockI2CBus();
//    IOPrint("TOUCHPAD RESET result=%d \n",error);
    return 0;
}

UInt8 I2CDevice::elan_smbus_set_mode(UInt8 mode)
{
    UInt8 cmd[4] = { 0x00, 0x07, 0x00, mode };
    LockI2CBus();
    int error = i2c_smbus_write_block_data(0x15,0, ETP_SMBUS_IAP_CMD,sizeof(cmd), cmd);
    UnlockI2CBus();
//    IOPrint("TOUCHPAD SET MODE ABSOLUTE: result %d\n",error);
    return error;
}


UInt8 I2CDevice::elan_smbus_get_report(UInt8 *report)
{
    SInt8 len;
    I2C_Transfer.op = I2CReadOp;
    I2C_Transfer.error_marker=0;
    LockI2CBus();
    len = i2c_smbus_read_block_data(0x15,0, ETP_SMBUS_PACKET_QUERY, &report[ETP_SMBUS_REPORT_OFFSET]);
   
    if (len < 0) {
     // failed to read report data
        fPCIDevice->ioWrite8(SMBSLVSTS(fBase), SMBSLVSTS_HST_NTFY_STS);
        UnlockI2CBus();
        return len;
    }
    
    if (len != ETP_SMBUS_REPORT_LEN) {
       fPCIDevice->ioWrite8(SMBSLVSTS(fBase), SMBSLVSTS_HST_NTFY_STS);
        UnlockI2CBus();
        return -EIO;
    }
    //clean hostnotify bit
    fPCIDevice->ioWrite8(SMBSLVSTS(fBase), SMBSLVSTS_HST_NTFY_STS);
    UnlockI2CBus();
    return 0;
}
