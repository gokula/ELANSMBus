//
//  Created by AS on 05/02/2019.
//  Copyright © 2019 Antonio Salado. All rights reserved.
//

#ifndef _I2CCommon_
#define _I2CCommon_

#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOFilterInterruptEventSource.h>
#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>
#include "Multitouch Support/VoodooI2CMultitouchInterfaceAS.hpp"
#include <IOKit/IOCommandGate.h>

namespace I2COperations {
typedef enum {
    I2CReadOp = 1,
    I2CWriteOp
} I2COp;
}
    bool read_in_progress;
    bool readingreport;

class I2CDevice: public IOService
{
    OSDeclareDefaultStructors(I2CDevice)
public:
    IOWorkLoop *MyWorkLoop;
    IOInterruptEventSource *fInterruptSrc;
    IOPCIDevice *fPCIDevice;
    UInt32 fBase;
    UInt8 fSt;
    UInt8 iSt;
    struct {
        IOLock *holder;
        bool event;
    } Lock;
    IORWLock *I2C_Lock;
    
    /* Misses serialization */
    struct {
        int op;
        bool error_marker;
        void *buffer;
        size_t length;
    } I2C_Transfer;
 
    /*
     * Data for SMBus Messages
     */
#define I2C_SMBUS_BLOCK_MAX    32    /* As specified in SMBus standard */
    union i2c_smbus_data {
        UInt8 byte;
        UInt16 word;
        UInt8 block[I2C_SMBUS_BLOCK_MAX + 2]; /* block[0] is used for length */
        /* and one more for user-space compatibility */
    };
    
    bool awake;
    bool ready_for_input;
    char device_name[10];
    char elan_name[5];
    int pressure_adjustment;
    int product_id;
    UInt8 reportData_last[34];
    IOCommandGate* command_gate;
    VoodooI2CMultitouchInterfaceAS *mt_interface;
    OSArray* transducers;
////asm
    virtual bool createWorkLoop(void);
    virtual IOWorkLoop *getWorkLoop(void) const;
    IOInterruptEventSource *CreateDeviceInterrupt(IOInterruptEventSource::Action,
                                                  IOFilterInterruptEventSource::Filter,
                                                  IOService *);
    int I2CExec(I2COperations::I2COp, UInt16, void *, size_t, void *, size_t);
protected:
    virtual bool    init (OSDictionary* dictionary = NULL);
    virtual void    free (void);
    virtual IOService*      probe (IOService* provider, SInt32* score);
    virtual bool    start (IOService* provider);
    virtual void    stop (IOService* provider);
    
public:
static    void interruptHandler(OSObject *owner, IOInterruptEventSource *src, int count);
static    bool interruptFilter(OSObject *owner, IOFilterInterruptEventSource *sender);
    
    void LockI2CBus();
    void UnlockI2CBus();
    int ReadI2CBus(UInt16, void *, size_t, void *, size_t);
    int WriteI2CBus(UInt16, void *, size_t, void *, size_t);
    int i801_check_pre(void);
    int i801_check_post(int status);
    int i801_wait_intr(void);
    int i801_transaction(UInt8 xact);
    SInt32 i2c_smbus_read_block_data(UInt8 addr, UInt8 flags, UInt8 command,
                                     UInt8 *values);
    SInt32 i2c_smbus_write_block_data(UInt8 addr, UInt8 flags, UInt8 command,
                                      UInt8 length, const UInt8 *values);
    SInt32 i2c_smbus_xfer( UInt16 addr,
                          unsigned short flags, char read_write,
                          UInt8 command, int protocol, union i2c_smbus_data *data);
    SInt32 __i2c_smbus_xfer( UInt16 addr,
                     unsigned short flags, char read_write,
                            UInt8 command, int protocol, union i2c_smbus_data *data);
    SInt32 i801_access( UInt16 addr,
                unsigned short flags, char read_write, UInt8 command,
                int size, union i2c_smbus_data *data);
   SInt32 i801_block_transaction(union i2c_smbus_data *data, char read_write,
                           int command, int hwpec);
    int i801_set_block_buffer_mode(void);
    SInt32 i801_block_transaction_by_block(
                                           union i2c_smbus_data *data,
                                           char read_write, int hwpec);
    int elan_smbus_initialize(void);
    int elan_smbus_probe(int address);
    SInt32 i2c_smbus_read_byte_data(UInt8 addr, UInt8 flags, UInt8 command,
                             UInt8 *values);
    
    SInt32 i2c_smbus_write_byte_data(UInt8 addr, UInt8 flags, UInt8 command,
                                     UInt8 *values);
    UInt8 elan_smbus_get_max(UInt32 *max_x, UInt32 *max_y);
    UInt8 elan_smbus_get_num_traces(UInt8 *x_traces, UInt8 *y_traces);
    UInt8 elan_smbus_get_resolution(UInt8 *hw_res_x, UInt8 *hw_res_y);
    UInt8 elan_smbus_reset(void);
    UInt8 elan_smbus_set_mode(UInt8 mode);
    UInt8 elan_smbus_get_report(UInt8 *report);
    IOReturn elan_get_report_gated(UInt8 *report) ;

    uint64_t maxaftertyping = 500000000;
    uint64_t keytime = 0;
    
    void handle_input_threaded();
    bool init_device();
    IOReturn parse_ELAN_report();
    IOReturn parse_ELAN_report2();

    bool publish_multitouch_interface();
    void release_resources();
    bool reset_device();
    void set_sleep_status(bool enable);
    void unpublish_multitouch_interface();
};
#endif
