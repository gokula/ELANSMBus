//
//  Created by AS on 05/02/2019.
//  Copyright © 2019 Antonio Salado. All rights reserved.
//

#include "I2CCommon.h"

#define ICH_DEBUG 1

#define drvid "[ICHSMBus] "
#define I2CNoOp 0
#define super IOService

#ifdef ICH_DEBUG
#define DbgPrint(arg...) IOLog(drvid arg)
#define PrintBitFieldExpanded(val) IOPrint("Register decoded: 0x%x<BUSY=%d,INTR=%d," \
"DEVERR=%d,BUSERR=%d,FAILED=%d,SMBAL=%d,INUSE=%d,BDONE=%d>\n", val, \
(val & ICH_SMB_HS_BUSY) != 0, (val & ICH_SMB_HS_INTR) != 0, \
(val & ICH_SMB_HS_DEVERR) != 0, (val & ICH_SMB_HS_BUSERR) != 0, \
(val & ICH_SMB_HS_FAILED) != 0, (val & ICH_SMB_HS_SMBAL) != 0, \
(val & ICH_SMB_HS_INUSE) != 0, (val & ICH_SMB_HS_BDONE) != 0)
#else
#define DbgPrint(arg...) ;
#define PrintBitFieldExpanded(val) ;
#endif
#define IOPrint(arg...) IOLog(drvid arg)

/* PCI configuration registers */
#define ICH_SMB_HOSTC 0x40
#define ICH_SMB_BASE    0x20
#define ICH_SMB_HOSTC_HSTEN    (1 << 0)
#define ICH_SMB_HOSTC_SMIEN    (1 << 1)

#define ICHSMBUS_DELAY 100
#define ICHIIC_TIMEOUT 1 /* in sec */

/* -------------- I801 SMBus address offsets ASM*/


/* I801 SMBus HOST STATUS */
#define SMBHSTSTS(p)    (p) /* host status */
/* I801 Hosts Status register bits */
#define SMBHSTSTS_BYTE_DONE    (1 << 7)        /* byte received/transmitted */
#define SMBHSTSTS_INUSE_STS    (1 << 6)        /* bus semaphore */
#define SMBHSTSTS_SMBALERT_STS    (1 << 5)   /* SMBALERT# asserted */
#define SMBHSTSTS_FAILED    (1 << 4)
#define SMBHSTSTS_BUS_ERR    (1 << 3)        /* transaction collision */
#define SMBHSTSTS_DEV_ERR    (1 << 2)        /* command error */
#define SMBHSTSTS_INTR        (1 << 1)       /* command completed */
#define SMBHSTSTS_HOST_BUSY    (1 << 0)       /* running a command */

/* I801 SMBus HOST CONTROL */
#define SMBHSTCNT(p)    (2 + p)
/* I801 Host Control register bits */
#define SMBHSTCNT_INTREN    (1 << 0)        /* enable interrupts */
#define SMBHSTCNT_KILL        (1 << 1)
#define ICH_SMB_HC_CMD_BYTE    (1 << 2)
#define ICH_SMB_HC_CMD_BDATA    (2 << 2)    /* BYTE DATA command */
#define ICH_SMB_HC_CMD_WDATA    (3 << 2)    /* WORD DATA command */
#define SMBHSTCNT_LAST_BYTE    (1 << 5)
#define SMBHSTCNT_START        (1 << 6)       /* start transaction */
#define SMBHSTCNT_PEC_EN    (1 << 7)        /* ICH3 and later */

/* I801 SMBus HOST COMMAND */
#define SMBHSTCMD(p)    (3 + p)
/* I801 SMBus HOST ADDRESS */
#define SMBHSTADD(p)    (4 + p)        /* transmit slave address */

#define ICH_SMB_TXSLVA_READ    (1 << 0)    /* read direction */
#define ICH_SMB_TXSLVA_ADDR(x)    (((x) & 0x7f) << 1) /* 7-bit address */


/* I801 SMBus HOST DATA 0 & 1 */
#define SMBHSTDAT0(p)    (5 + p)        /* host data 0 */
#define SMBHSTDAT1(p)    (6 + p)        /* host data 1 */

#define SMBBLKDAT(p)    (7 + p)
#define SMBPEC(p)        (8 + p)        /* ICH3 and later */
#define SMBAUXSTS(p)    (12 + p)    /* ICH4 and later */
#define SMBAUXCTL(p)    (13 + p)    /* ICH4 and later */
#define SMBSLVSTS(p)    (16 + p)    /* ICH3 and later */
#define SMBSLVCMD(p)    (17 + p)    /* ICH3 and later */
#define SMBNTFDADD(p)    (20 + p)    /* ICH3 and later */

/* PCI Address Constants */
#define SMBBAR        4
#define SMBPCICTL    0x004
#define SMBPCISTS    0x006
#define SMBHSTCFG    0x040
#define TCOBASE        0x050
#define TCOCTL        0x054

#define ACPIBASE        0x040
#define ACPIBASE_SMI_OFF    0x030
#define ACPICTRL        0x044
#define ACPICTRL_EN        0x080

#define SBREG_BAR        0x10
#define SBREG_SMBCTRL        0xc6000c
#define SBREG_SMBCTRL_DNV    0xcf000c

/* Host status bits for SMBPCISTS */
#define SMBPCISTS_INTS        (1 << 3)

/* Control bits for SMBPCICTL */
#define SMBPCICTL_INTDIS    (1 << 10)

/* Host configuration bits for SMBHSTCFG */
#define SMBHSTCFG_HST_EN    (1 << 0)
#define SMBHSTCFG_SMB_SMI_EN    (1 << 1)
#define SMBHSTCFG_I2C_EN    (1 << 2)
#define SMBHSTCFG_SPD_WD    (1 << 4)


/* TCO configuration bits for TCOCTL */
#define TCOCTL_EN         (1 << 8)

/* Auxiliary status register bits, ICH4+ only */
#define SMBAUXSTS_CRCE         (1 << 0)
#define SMBAUXSTS_STCO         (1 << 1)

/* Auxiliary control register bits, ICH4+ only */
#define SMBAUXCTL_CRC         (1 << 0)
#define SMBAUXCTL_E32B        (1 << 1)

/* Other settings */
#define MAX_RETRIES        400

/* I801 command constants */
#define I801_QUICK        0x00
#define I801_BYTE        0x04
#define I801_BYTE_DATA        0x08
#define I801_WORD_DATA        0x0C
#define I801_PROC_CALL        0x10    /* unimplemented */
#define I801_BLOCK_DATA        0x14
#define I801_I2C_BLOCK_DATA    0x18    /* ICH5 and later */

/* Host Notify Status register bits */
#define SMBSLVSTS_HST_NTFY_STS     (1 << 0)

/* Host Notify Command register bits */
#define SMBSLVCMD_HST_NTFY_INTREN     (1 << 0)

#define STATUS_ERROR_FLAGS    (SMBHSTSTS_FAILED | SMBHSTSTS_BUS_ERR | \
SMBHSTSTS_DEV_ERR)

#define STATUS_FLAGS        (SMBHSTSTS_BYTE_DONE | SMBHSTSTS_INTR | \
STATUS_ERROR_FLAGS)


#define FEATURE_SMBUS_PEC     (1 << 0)
#define FEATURE_BLOCK_BUFFER     (1 << 1)
#define FEATURE_BLOCK_PROC     (1 << 2)
#define FEATURE_I2C_BLOCK_READ     (1 << 3)
#define FEATURE_IRQ         (1 << 4)
#define FEATURE_HOST_NOTIFY     (1 << 5)
/* Not really a feature, but it's convenient to handle it as such */
#define FEATURE_IDF         (1 << 15)
#define FEATURE_TCO         (1 << 16)

#define I2C_FUNC_SMBUS_BYTE        (I2C_FUNC_SMBUS_READ_BYTE | \
I2C_FUNC_SMBUS_WRITE_BYTE)
#define I2C_FUNC_SMBUS_BYTE_DATA    (I2C_FUNC_SMBUS_READ_BYTE_DATA | \
I2C_FUNC_SMBUS_WRITE_BYTE_DATA)
#define I2C_FUNC_SMBUS_WORD_DATA    (I2C_FUNC_SMBUS_READ_WORD_DATA | \
I2C_FUNC_SMBUS_WRITE_WORD_DATA)
#define I2C_FUNC_SMBUS_BLOCK_DATA    (I2C_FUNC_SMBUS_READ_BLOCK_DATA | \
I2C_FUNC_SMBUS_WRITE_BLOCK_DATA)
#define I2C_FUNC_SMBUS_I2C_BLOCK    (I2C_FUNC_SMBUS_READ_I2C_BLOCK | \
I2C_FUNC_SMBUS_WRITE_I2C_BLOCK)

#define I2C_FUNC_SMBUS_EMUL        (I2C_FUNC_SMBUS_QUICK | \
I2C_FUNC_SMBUS_BYTE | \
I2C_FUNC_SMBUS_BYTE_DATA | \
I2C_FUNC_SMBUS_WORD_DATA | \
I2C_FUNC_SMBUS_PROC_CALL | \
I2C_FUNC_SMBUS_WRITE_BLOCK_DATA | \
I2C_FUNC_SMBUS_I2C_BLOCK | \
I2C_FUNC_SMBUS_PEC)

/*
 * Data for SMBus Messages
 */
#define I2C_SMBUS_BLOCK_MAX    32    /* As specified in SMBus standard */

/* i2c_smbus_xfer read or write markers */
#define I2C_SMBUS_READ    1
#define I2C_SMBUS_WRITE    0

/* SMBus transaction types (size parameter in the above functions)
 Note: these no longer correspond to the (arbitrary) PIIX4 internal codes! */
#define I2C_SMBUS_QUICK            0
#define I2C_SMBUS_BYTE            1
#define I2C_SMBUS_BYTE_DATA        2
#define I2C_SMBUS_WORD_DATA        3
#define I2C_SMBUS_PROC_CALL        4
#define I2C_SMBUS_BLOCK_DATA        5
#define I2C_SMBUS_I2C_BLOCK_BROKEN  6
#define I2C_SMBUS_BLOCK_PROC_CALL   7        /* SMBus 2.0 */
#define I2C_SMBUS_I2C_BLOCK_DATA    8

#define I2C_CLIENT_PEC        0x04    /* Use Packet Error Checking */

#define    EIO         5    /* I/O error */
#define    ENXIO         6    /* No such device or address */
#define    EAGAIN        11    /* Try again */
#define    EBUSY        16    /* Device or resource busy */
#define    EINVAL        22    /* Invalid argument */
#define    ETIMEDOUT    60    /* Connection timed out */
#define    EBADMSG        84    /* Not a data message */
#define    EOPNOTSUPP    45    /* Operation not supported on transport endpoint */
#define    EPROTO        85    /* Protocol error */
