/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: m47_drv.c
 *      Project: M47 module driver (MDIS5)
 *
 *       Author: ag
 *
 *  Description: Low-level driver for M47 M-Modules
 *
 *               The M47 M-Module is a Serial Synchronous Interface (SSI)
 *               M-Module with 4 input ports and interrupt capabilities.
 *
 *               The driver handles the M47 input ports as 4 channels:
 *               channel 0..3.
 *               
 *               The directions of the channels cannot be altered.
 *
 *               Usage of interrupts is not implemented in this driver.
 *               
 *               
 *
 *     Required: OSS, DESC, DBG, ID libraries 
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
 *
 *
 *---------------------------------------------------------------------------
 * Copyright 2003-2019, MEN Mikro Elektronik GmbH
 ****************************************************************************/

 /*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _NO_LL_HANDLE       /* ll_defs.h: don't define LL_HANDLE struct */

#include <MEN/men_typs.h>   /* system dependent definitions   */
#include <MEN/maccess.h>    /* hw access macros and types     */
#include <MEN/dbg.h>        /* debug functions                */
#include <MEN/oss.h>        /* oss functions                  */
#include <MEN/desc.h>       /* descriptor functions           */
#include <MEN/modcom.h>     /* ID PROM functions              */
#include <MEN/mdis_api.h>   /* MDIS global defs               */
#include <MEN/mdis_com.h>   /* MDIS common defs               */
#include <MEN/mdis_err.h>   /* MDIS error codes               */
#include <MEN/ll_defs.h>    /* low-level driver definitions   */
#include "m47_flex.h"       /* flex array */

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/* general */
#define CH_NUMBER           4           /* number of device channels */
#define USE_IRQ             FALSE       /* interrupt required  */
#define ADDRSPACE_COUNT     1           /* nr of required address spaces */
#define ADDRSPACE_SIZE      256         /* size of address space */
#define MOD_ID_MAGIC        0x5346      /* ID PROM magic word */
#define MOD_ID_SIZE         128         /* ID PROM size [bytes] */
#define MOD_ID              47          /* ID PROM M-Module ID */
#define DATABUFSIZE         32          /* size of data buffer + reserved area */
#define CONT_DEFAULT        0x00000080  /* default value of control register: */
                                        /* data width 32 bits, baud rate 500kbaud */
#define MODE_DEFAULT        0x00000000  /* default value of transmission mode: Gray encoding */

#define HW_MAJOR_REV_2      0x0200      /* HW major revision 2 */

/* debug settings */
#define DBG_MYLEVEL         llHdl->dbgLevel
#define DBH                 llHdl->dbgHdl

/* register offsets */

#define CONTREG_CH0         0x80        /* Control Register offset CH 0 */
#define CONTREG_CH1         0x82        /* Control Register offset CH 1 */
#define CONTREG_CH2         0x88        /* Control Register offset CH 2 */
#define CONTREG_CH3         0x8c        /* Control Register offset CH 3 */

#define MODE_REV_CH0        0x84        /* Mode/PLD Revision Register CH 0 */
#define MODE_REV_CH1        0x86        /* Mode/PLD Revision Register CH 1 */
#define MODE_REV_CH2        0x8a        /* Mode/PLD Revision Register CH 2 */
#define MODE_REV_CH3        0x8e        /* Mode/PLD Revision Register CH 3 */

#define FLEXREG             0xde        /* offset for flex load */
#define STATUS_REG          0xa0        /* Status Register offset */
#define REG_START           0x00        /* Data Register offset for channel */
#define DATACH(ch)          ((ch) << 3) /* register offset for single channels */
/*
#define DATACH(ch)          ((ch * 8) + 1)
*/

/* single bit setting macros used by m47_flexload */
#define bitset(byte,mask)  ((byte) |=  (mask))
#define bitclr(byte,mask)  ((byte) &= ~(mask))
#define bitmove(byte,mask,bool) (bool ? bitset(byte,mask) : bitclr(byte,mask))


/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/

/* structure of M47 driver options */
typedef struct {
    u_int16         dataWidth;      /* number of bits in a data word = data width */
    u_int16         baudRate;       /* baud rate for data transmission */
    u_int16         transMode;      /* transmission mode = sensor encoding (Gray or binary) */
} M47_OPTIONS;

/* low-level handle */
typedef struct {
    /* general */
    int32           memAlloc;       /* size allocated for the handle */
    OSS_HANDLE      *osHdl;         /* oss handle */
    OSS_IRQ_HANDLE  *irqHdl;        /* irq handle */
    DESC_HANDLE     *descHdl;       /* desc handle */
    MACCESS         ma;             /* hw access handle */
    MDIS_IDENT_FUNCT_TBL idFuncTbl; /* id function table */
    /* debug */
    u_int32         dbgLevel;       /* debug level */
    DBG_HANDLE      *dbgHdl;        /* debug handle */
    /* misc */
    u_int32         irqCount;       /* interrupt counter */
    u_int32         idCheck;        /* id check enabled */
    M47_OPTIONS     options[CH_NUMBER];     /* structure of M47 driver options */
    u_int16         moduleHwRev;            /* HW Revision from Module EEPROM */
} LL_HANDLE;

    

/* include files which need LL_HANDLE */
#include <MEN/ll_entry.h>  /* low-level driver jump table  */
#include <MEN/m47_drv.h>   /* M47 driver header file */

static const char IdentString[]=MENT_XSTR(MAK_REVISION);

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
static int32 m47_flexload ( LL_HANDLE *llHdl );
static int32 M47_Init(DESC_SPEC *descSpec, OSS_HANDLE *osHdl,
                       MACCESS *ma, OSS_SEM_HANDLE *devSemHdl,
                       OSS_IRQ_HANDLE *irqHdl, LL_HANDLE **llHdlP);
static int32 M47_Exit(LL_HANDLE **llHdlP );
static int32 M47_Read(LL_HANDLE *llHdl, int32 ch, int32 *value);
static int32 M47_Write(LL_HANDLE *llHdl, int32 ch, int32 value);
static int32 M47_SetStat(LL_HANDLE *llHdl,int32 ch, int32 code, INT32_OR_64 value32_or_64 );
static int32 M47_GetStat(LL_HANDLE *llHdl, int32 ch, int32 code, INT32_OR_64 *value32_or_64P );
static int32 M47_BlockRead(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
                            int32 *nbrRdBytesP);
static int32 M47_BlockWrite(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
                             int32 *nbrWrBytesP);
static int32 M47_Irq(LL_HANDLE *llHdl );
static int32 M47_Info(int32 infoType, ... );

static char* Ident( void );
static int32 Cleanup(LL_HANDLE *llHdl, int32 retCode);
static char* M47_FlexDataIdent( void );
static void M47_UpdateControlRegs( LL_HANDLE *llHdl );

/******************************** m47_flexload *******************************
 *
 *  Description:  Load FLEXlogic (JTAG interface) with binary data.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl    low-level handle
 *  Output.....:  return   0
 *  Globals....:  ---
 ****************************************************************************/

static int32 m47_flexload
(
    LL_HANDLE *llHdl
    
)
{
   u_int8  ctrl,                   /* control word */
           ch, n;                  /* current byte */
   u_int8  tdo;
   u_int8  tck;
   u_int8  tms;
   u_int8  *p = (u_int8*)M47_FlexData;     /* point to binary data */
   u_int32 size;                           /* size of binary data */

   DBGWRT_1((DBH, "LL - m47_flexload\n"));

   size  = (u_int32)(*p++) << 24;   /* get data size */
   size |= (u_int32)(*p++) << 16;
   size |= (u_int32)(*p++) <<  8;
   size |= (u_int32)(*p++);

   tdo = 1 << 0;         /* convert bits to bitmask */
   tck = 1 << 1;
   tms = 1 << 2;

   ctrl = 0x00;            /* control word initial value */

   while(size--)           /* for all bytes */
   {
      ch = *p++;              /* get next data byte */
      n = 4;                  /* data byte: 4*2 bit */

      while(n--)                 /* write data 2 bits */
      {

         bitclr (ctrl,tck);                 /* clear TCK */
         MWRITE_D16( llHdl->ma, FLEXREG, ctrl );

         bitmove(ctrl,tdo,ch & 0x01);       /* write TDO/TMS bits */
         bitmove(ctrl,tms,ch & 0x02);
         MWRITE_D16( llHdl->ma, FLEXREG, ctrl );

         bitset (ctrl,tck);                 /* set TCK (pulse) */
         MWRITE_D16( llHdl->ma, FLEXREG, ctrl );

         ch >>= 2;
      }
   }

   return(0);
}

/**************************** M47_GetEntry *********************************
 *
 *  Description:  Initialize driver's jump table
 *
 *---------------------------------------------------------------------------
 *  Input......:  ---
 *  Output.....:  drvP  pointer to the initialized jump table structure
 *  Globals....:  ---
 ****************************************************************************/
#ifdef _ONE_NAMESPACE_PER_DRIVER_
    extern void LL_GetEntry( LL_ENTRY* drvP )
#else
# ifndef MAC_BYTESWAP
    extern void M47_GetEntry( LL_ENTRY* drvP )
# else
    extern void M47_SW_GetEntry( LL_ENTRY* drvP )
# endif /* MAC_BYTESWAP */
#endif
{
    drvP->init        = M47_Init;
    drvP->exit        = M47_Exit;
    drvP->read        = M47_Read;
    drvP->write       = M47_Write;
    drvP->blockRead   = M47_BlockRead;
    drvP->blockWrite  = M47_BlockWrite;
    drvP->setStat     = M47_SetStat;
    drvP->getStat     = M47_GetStat;
    drvP->irq         = M47_Irq;
    drvP->info        = M47_Info;
}

/******************************** M47_Init ***********************************
 *
 *  Description:  Allocate and return low-level handle, initialize hardware
 * 
 *                The function initializes all channels with the 
 *                definitions made in the descriptor. The interrupt 
 *                is always disabled.
 *
 *                The following descriptor keys are used:
 *
 *                Descriptor key        Default          Range
 *                --------------------  ---------------  -------------
 *                DEBUG_LEVEL_DESC      OSS_DBG_DEFAULT  see dbg.h
 *                DEBUG_LEVEL           OSS_DBG_DEFAULT  see dbg.h
 *                ID_CHECK              1                0..1
 *                M47_CONTROL           0x00000080       see below
 *                M47_TRANSMODE         0x00000000       0x00000000 (Gray)
 *                                                       0x00000080 (binary)
 *
 *                M47_CONTROL sets the baud rate and number of bits in
 *                    a data word:
 *
 *                    Bit       31..8         7..2      1..0
 *                         +----------------------------------+ 
 *                         | 0   ...   0 |     BW    |   BR   |
 *                         +----------------------------------+
 *
 *                    BW: Number of bits in data word (bit width), 1..32
 *                        0 0 0 0 0 0 = Stop transmission
 *                        0 0 0 0 0 1 = 1 bit in data word
 *                        ..
 *                        1 0 0 0 0 0 = 32 bits in data word
 *                        Other values are not permitted!
 *
 *                    BR: 0 0 = 500 kbaud
 *                        0 1 = 250 kbaud
 *                        1 0 = 125 kbaud
 *                        1 1 = 62.5 kbaud
 *---------------------------------------------------------------------------
 *  Input......:  descSpec   pointer to descriptor data
 *                osHdl      oss handle
 *                ma         hardware access handle
 *                devSemHdl  device semaphore handle
 *                irqHdl     irq handle
 *  Output.....:  llHdlP     pointer to low-level driver handle
 *                return     success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M47_Init(
    DESC_SPEC       *descP,
    OSS_HANDLE      *osHdl,
    MACCESS         *ma,
    OSS_SEM_HANDLE  *devSemHdl,
    OSS_IRQ_HANDLE  *irqHdl,
    LL_HANDLE       **llHdlP
)
{
    LL_HANDLE *llHdl = NULL;
    u_int32 gotsize;
    int32 error;
    u_int32 value;
    u_int16 count;
    u_int16 n;          /* count for data buffer clearing */
    u_int32 contReg;    /* control register entry read from descriptor */
    u_int32 modeReg;    /* mode register entry read from descriptor */

    count = 0;
    n     = DATABUFSIZE;
    
    /*------------------------------+
    |  prepare the handle           |
    +------------------------------*/
    *llHdlP = NULL;        /* set low-level driver handle to NULL */ 
    
    /* alloc */
    if ((llHdl = (LL_HANDLE*)OSS_MemGet(
                    osHdl, sizeof(LL_HANDLE), &gotsize)) == NULL)
       return(ERR_OSS_MEM_ALLOC);

    /* clear */
    OSS_MemFill(osHdl, gotsize, (char*)llHdl, 0x00);

    /* init */
    llHdl->memAlloc   = gotsize;
    llHdl->osHdl      = osHdl;
    llHdl->irqHdl     = irqHdl;
    llHdl->ma         = *ma;

    /*------------------------------+
    |  init id function table       |
    +------------------------------*/
    /* driver's ident function */
    llHdl->idFuncTbl.idCall[0].identCall = Ident;
    llHdl->idFuncTbl.idCall[1].identCall = M47_FlexDataIdent;    
    /* library's ident functions */
    llHdl->idFuncTbl.idCall[2].identCall = DESC_Ident;
    llHdl->idFuncTbl.idCall[3].identCall = OSS_Ident;
    /* terminator */
    llHdl->idFuncTbl.idCall[4].identCall = NULL;

    /*------------------------------+
    |  prepare debugging            |
    +------------------------------*/
    DBG_MYLEVEL = OSS_DBG_DEFAULT;  /* set OS specific debug level */
    DBGINIT((NULL,&DBH));

    /*------------------------------+
    |  scan descriptor              |
    +------------------------------*/
    /* prepare access */
    if ((error = DESC_Init(descP, osHdl, &llHdl->descHdl)))
        return( Cleanup(llHdl,error) );

    /* DEBUG_LEVEL_DESC */
    if ((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT, 
                                &value, "DEBUG_LEVEL_DESC")) &&
        error != ERR_DESC_KEY_NOTFOUND)
        return( Cleanup(llHdl,error) );

    DESC_DbgLevelSet(llHdl->descHdl, value);    /* set level */

    /* DEBUG_LEVEL */
    if ((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT, 
                                &llHdl->dbgLevel, "DEBUG_LEVEL")) &&
        error != ERR_DESC_KEY_NOTFOUND)
        return( Cleanup(llHdl,error) );

    DBGWRT_1((DBH, "LL - M47_Init\n"));

    /* ID_CHECK */
    if ((error = DESC_GetUInt32(llHdl->descHdl, TRUE, 
                                &llHdl->idCheck, "ID_CHECK")) &&
        error != ERR_DESC_KEY_NOTFOUND)
        return( Cleanup(llHdl,error) );

    /* M47_CONTROL */
    if ((error = DESC_GetUInt32(llHdl->descHdl, CONT_DEFAULT, 
                                &contReg, "M47_CONTROL")) &&
        error != ERR_DESC_KEY_NOTFOUND)
        return( Cleanup(llHdl,error) );

    /* M47_TRANSMODE */
    if ((error = DESC_GetUInt32(llHdl->descHdl, MODE_DEFAULT, 
                                &modeReg, "M47_TRANSMODE")) &&
        error != ERR_DESC_KEY_NOTFOUND)
        return( Cleanup(llHdl,error) );

    DBGWRT_2((DBH, "LL - modeReg = %08\n", modeReg));
    
    /* set M47 option structure */
    llHdl->options[0].transMode =
    llHdl->options[1].transMode =
    llHdl->options[2].transMode =
    llHdl->options[3].transMode = (u_int16)(modeReg >> 7);
    
    llHdl->options[0].dataWidth =
    llHdl->options[1].dataWidth =
    llHdl->options[2].dataWidth =
    llHdl->options[3].dataWidth = (u_int16)(contReg >> 2);

    llHdl->options[0].baudRate =
    llHdl->options[1].baudRate =
    llHdl->options[2].baudRate =
    llHdl->options[3].baudRate = (u_int16)( (contReg & 0x00000003) );

    /* check if option structure contains valid data */
    if ( llHdl->options[0].transMode > 1 )
    {        
        error = ERR_LL_DESC_PARAM;
        DBGWRT_ERR((DBH," *** M47_Init: illegal descriptor parameter" 
        "transmission mode= %d\n", 
        llHdl->options[0].transMode ));
        return ( Cleanup(llHdl,error) );
    }

    if ( llHdl->options[0].dataWidth > 32 )
    {        
        error = ERR_LL_DESC_PARAM;
        DBGWRT_ERR((DBH," *** M47_Init: illegal descriptor parameter" 
        "data-width = %d\n", 
        llHdl->options[0].dataWidth ));
        return ( Cleanup(llHdl,error) );
    }
    
    if ( llHdl->options[0].baudRate > 3 )
    {        
        error = ERR_LL_DESC_PARAM;
        DBGWRT_ERR((DBH," *** M47_Init: illegal descriptor parameter" 
        "baudrate = %d\n", 
        llHdl->options[0].baudRate ));
        return ( Cleanup(llHdl,error) );
    }

    DBGWRT_2((DBH, "LL - modeTrans = %d\n", llHdl->options[0].transMode));
    DBGWRT_2((DBH, "LL - dataWidth = %d\n", llHdl->options[0].dataWidth));
    DBGWRT_2((DBH, "LL - baudRate = %d\n", llHdl->options[0].baudRate));

    /*------------------------------+
    |  check module ID              |
    +------------------------------*/
    if (llHdl->idCheck) {
        int modIdMagic = m_read((U_INT32_OR_64)llHdl->ma, 0);
        int modId      = m_read((U_INT32_OR_64)llHdl->ma, 1);
        DBGWRT_2((DBH, "LL - EEPROM read\n"));
        if (modIdMagic != MOD_ID_MAGIC) {
            DBGWRT_ERR((DBH," *** M47_Init: illegal magic=0x%04x\n",modIdMagic));
            error = ERR_LL_ILL_ID;
            return( Cleanup(llHdl,error) );
        }
        if (modId != MOD_ID) {
            DBGWRT_ERR((DBH," *** M47_Init: illegal id=%d\n",modId));
            error = ERR_LL_ILL_ID;
            return( Cleanup(llHdl,error) );
        }
    }

    /*------------------------------+
    |  Read module revision number  |
    +------------------------------*/
    llHdl->moduleHwRev = (u_int16) m_read((U_INT32_OR_64)llHdl->ma, 2);
    DBGWRT_1((DBH, "LL - M47 HW Revision = 0x%04X\n", llHdl->moduleHwRev ));

    /*------------------------------+
    |  init hardware                |
    +------------------------------*/
    

    DBGWRT_3((DBH, "LL - Mode/PLD Revision Register = %04X\n", 
    MREAD_D16( llHdl->ma, MODE_REV_CH0)));

    /* Flex load */
    m47_flexload(llHdl);
    DBGWRT_2((DBH, "LL - Flex loaded\n"));
    
    
    DBGWRT_2((DBH, "LL - Mode/PLD Revision Register = %04X\n", 
    MREAD_D16( llHdl->ma, MODE_REV_CH0)));
    

    DBGWRT_2((DBH, "LL - Read Control Register\n"
                   " CONTREG_CH0 = %04X\n"
                   " CONTREG_CH1 = %04X\n"
                   " CONTREG_CH2 = %04X\n"
                   " CONTREG_CH3 = %04X\n", 
                   MREAD_D16( llHdl->ma, CONTREG_CH0),
                   MREAD_D16( llHdl->ma, CONTREG_CH1),
                   MREAD_D16( llHdl->ma, CONTREG_CH2),
                   MREAD_D16( llHdl->ma, CONTREG_CH3)));

    /* Stop transmission */
    MWRITE_D16( llHdl->ma, CONTREG_CH0, 0x0000 );
    DBGWRT_2((DBH, "LL - Transmission stopped\n"
                   " CONTREG_CH0 = %04X\n"
                   " CONTREG_CH1 = %04X\n"
                   " CONTREG_CH2 = %04X\n"
                   " CONTREG_CH3 = %04X\n", 
                   MREAD_D16( llHdl->ma, CONTREG_CH0),
                   MREAD_D16( llHdl->ma, CONTREG_CH1),
                   MREAD_D16( llHdl->ma, CONTREG_CH2),
                   MREAD_D16( llHdl->ma, CONTREG_CH3)));
    
    /* Clear data RAM */    
    while (n--)
    {
        MWRITE_D16( llHdl->ma, (REG_START + count), 0x0000);
        count +=2;
    }

    DBGWRT_2((DBH, "LL - RAM Cleared\n"));
    
    /* Write mode to Mode/PLD Revision Register */
    MWRITE_D16( llHdl->ma, MODE_REV_CH0, ((u_int16) modeReg));

    DBGWRT_2((DBH, "LL - Mode/PLD Revision Register (desc. value set)\n"
                   " MODE_REV_CH0 = %04X\n"
                   " MODE_REV_CH1 = %04X\n"
                   " MODE_REV_CH2 = %04X\n"
                   " MODE_REV_CH3 = %04X\n", 
                   MREAD_D16( llHdl->ma, MODE_REV_CH0),
                   MREAD_D16( llHdl->ma, MODE_REV_CH1),
                   MREAD_D16( llHdl->ma, MODE_REV_CH2),
                   MREAD_D16( llHdl->ma, MODE_REV_CH3)));

    /* Load Control Register */
    MWRITE_D16( llHdl->ma, CONTREG_CH0, ((u_int16) contReg) );

    DBGWRT_2((DBH, "LL - Descriptor value M47_CONTROL = %08X\n", contReg));
    DBGWRT_2((DBH, "LL - Read Controll Register (desc. value set)\n"
                   " CONTREG_CH0 = %04X\n"
                   " CONTREG_CH1 = %04X\n"
                   " CONTREG_CH2 = %04X\n"
                   " CONTREG_CH3 = %04X\n", 
                   MREAD_D16( llHdl->ma, CONTREG_CH0),
                   MREAD_D16( llHdl->ma, CONTREG_CH1),
                   MREAD_D16( llHdl->ma, CONTREG_CH2),
                   MREAD_D16( llHdl->ma, CONTREG_CH3)));
    
    *llHdlP = llHdl;    /* set low-level driver handle */

    return(ERR_SUCCESS);
}

/****************************** M47_Exit *************************************
 *
 *  Description:  De-initialize hardware and clean up memory
 *
 *                The function stops the transmission by setting the data
 *                width to 0 (i.e. it writes 0x0000 to the Control Register).
 *                The interrupt is disabled.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdlP    pointer to low-level driver handle
 *  Output.....:  return    success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M47_Exit(
   LL_HANDLE    **llHdlP
)
{
    LL_HANDLE *llHdl = *llHdlP;
    int32 error = 0;

    DBGWRT_1((DBH, "LL - M47_Exit\n"));

    /*------------------------------+
    |  de-init hardware             |
    +------------------------------*/

    /* Stop Transmission */
    MWRITE_D16( llHdl->ma, CONTREG_CH0, 0x0000 );
    
    /*------------------------------+
    |  clean up memory               |
    +------------------------------*/
    *llHdlP = NULL;        /* set low-level driver handle to NULL */ 
    error = Cleanup(llHdl,error);

    return(error);
}

/****************************** M47_Read *************************************
 *
 *  Description:  Read a value from the device
 *
 *                The function reads the input state of the current channel.
 *                The routine writes its read value into a variable of 
 *                type unsigned long. The bits beginning from the least 
 *                significant bit up to the maximum number of bits (defined
 *                by the data width) are valid.
 *
 *                Bit  32 ...      max ...                 0
 *                    +-------------------------------------+
 *                    |  reserved  |     valid data         |
 *                    +-------------------------------------+
 *---------------------------------------------------------------------------
 *  Input......:  llHdl    low-level handle
 *                ch       current channel
 *  Output.....:  valueP   read value
 *                return   success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M47_Read(
    LL_HANDLE *llHdl,
    int32 ch,
    int32 *valueP
)
{
    register u_int32   data;
    register u_int8    dummy;


    data = 0;

    DBGWRT_1((DBH, "LL - M47_Read: ch=%d\n",ch));

    DBGDMP_2((DBH,"REGS",(void *)llHdl->ma,0x20,2));    

    /* Read d32..d24 */
    dummy = ((u_int8) MREAD_D16 (llHdl->ma, DATACH(ch)));
    data |= dummy;
    data <<= 8;

    DBGWRT_2((DBH, "LL - M47_Read: data=%08X\n", data));
    
    /* Read d23..d16 */
    dummy = ((u_int8) MREAD_D16 (llHdl->ma, (DATACH(ch) + 0x02)));
    data |= dummy;
    data <<= 8;

    DBGWRT_2((DBH, "LL - M47_Read: data=%08X\n", data));

    /* Read d15..d8 */
    dummy = ((u_int8) MREAD_D16 (llHdl->ma, (DATACH(ch) + 0x04)));
    data |= dummy;
    data <<= 8;
    
    DBGWRT_2((DBH, "LL - M47_Read: data=%08X\n", data));

    /* Read d8..d0 */
    
    dummy = ((u_int8) MREAD_D16 (llHdl->ma, (DATACH(ch) + 0x06)));
    data |= dummy;
    
    DBGWRT_2((DBH, "LL - M47_Read: data=%08X\n", data));

    *valueP = data;

    return(ERR_SUCCESS);
}

/****************************** M47_Write ************************************
 *
 *  Description:  Do nothing
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl    low-level handle
 *                ch       current channel
 *                value    value to write 
 *  Output.....:  return   error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M47_Write(
    LL_HANDLE *llHdl,
    int32 ch,
    int32 value
)
{
    DBGWRT_1((DBH, "LL - M47_Write: Function not implemented ch=%d\n",ch));
    
    return(ERR_LL_WRITE);
}

/****************************** M47_SetStat **********************************
 *
 *  Description:  Set the driver status
 *
 *                The following status codes are supported:
 *
 *                Code                 Description                 Values
 *                -------------------  --------------------------  ----------
 *                M_LL_DEBUG_LEVEL     driver debug level          see dbg.h
 *                M_MK_IRQ_ENABLE      interrupt enable            0..1
 *                M_LL_IRQ_COUNT       interrupt counter           0..max
 *                M_LL_CH_DIR          direction of curr. chan.    M_CH_???
 *                M47_BAUDRATE         baud rate for SSI device    0..3
 *                M47_DATA_WIDTH       data width                  0..32
 *                M47_TRANS_MODE       transmission mode           0 (Gray) 
 *                                                                   1 (binary)
 *
 *                M47_BAUDRATE sets the baud rate for the SSI device:
 *                    0 = 500 kbaud
 *                    1 = 250 kbaud
 *                    2 = 125 kbaud
 *                    3 = 62.5 kbaud
 *                    (see also m47_drv.h #define M47_BAUD_XXX)
 *
 *                The following codes are only valid in conjunction with M47
 *                HW revision 2.0 or higher!
 *
 *                M47_BAUDRATE_CH      baudrate for specific CH    see 
 *                                                                 M47_BAUDRATE
 *                M47_DATA_WIDTH_CH    data width for specific CH  0..32
 *                M47_TRANS_MODE_CH    trans. mode for specific CH 0 (Gray) 
 *                                                                 1 (binary)
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl         low-level handle
 *                code          status code
 *                ch            current channel
 *                value32_or_64 data or
 *                              pointer to block data structure (M_SG_BLOCK)(*)
 *                              (*) = for block status codes
 *  Output.....:  return     success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M47_SetStat(
    LL_HANDLE *llHdl,
    int32  code,
    int32  ch,
    INT32_OR_64 value32_or_64
)
{
    int32 error = ERR_SUCCESS;
    u_int16 n;
    u_int16 count;
    
    int32 value = (int32)value32_or_64; /* 32bit value */
    
    count = 0;
    n     = DATABUFSIZE;

    DBGWRT_1((DBH, "LL - M47_SetStat: ch=%d code=0x%04x value=0x%x\n",
              ch,code,value));

    switch(code) {
        /*--------------------------+
        |  debug level              |
        +--------------------------*/
        case M_LL_DEBUG_LEVEL:
            llHdl->dbgLevel = value;
            break;
        /*--------------------------+
        |  enable interrupts        |
        +--------------------------*/
        case M_MK_IRQ_ENABLE:
            error = ERR_LL_UNK_CODE;    /* say: not supported */
            break;
        /*--------------------------+
        |  set irq counter          |
        +--------------------------*/
        case M_LL_IRQ_COUNT:
            llHdl->irqCount = value;
            break;
        /*--------------------------+
        |  channel direction        |
        +--------------------------*/
        case M_LL_CH_DIR:
            switch(value) {
                case M_CH_OUT:
                    /* all channels are inputs */
                    error = ERR_LL_ILL_DIR;
                    break;
                case M_CH_IN:
                    /* all channels are inputs */
                    
                    break;
                case M_CH_INOUT:
                    /* all channels are inputs */
                    error = ERR_LL_ILL_DIR;
                    break;
                default:
                    error = ERR_LL_ILL_DIR;
            }

            break;

        /*-------------------------------------+
        |  set M47 baud rate for all channels  |
        +-------------------------------------*/
        case M47_BAUDRATE:
            
            if ( value < 0 || value > 3) {
                error = ERR_LL_ILL_PARAM;
                break;
            }
                
            llHdl->options[0].baudRate =
            llHdl->options[1].baudRate =
            llHdl->options[2].baudRate =
            llHdl->options[3].baudRate = (u_int16) value;
            
            /* stop transmission */
            MWRITE_D16( llHdl->ma, CONTREG_CH0, 0x0000 );

               /* clear data RAM */    
            while (n--)
            {
                MWRITE_D16( llHdl->ma, (REG_START + count), 0x0000);
                count +=2;
            }
            
            MWRITE_D16(llHdl->ma, CONTREG_CH0, ((llHdl->options[0].baudRate) | 
            (llHdl->options[0].dataWidth << 2)));
            
            break;

        /*--------------------------------------+
        |  set M47 data width for all channels  |
        +--------------------------------------*/
        case M47_DATA_WIDTH:

            if(value < 0 || value > 32)
            {
                error = ERR_LL_ILL_PARAM;
                break;
            }
            
            llHdl->options[0].dataWidth =
            llHdl->options[1].dataWidth =
            llHdl->options[2].dataWidth =
            llHdl->options[3].dataWidth = (u_int16) value;

            /* stop transmission */
            MWRITE_D16( llHdl->ma, CONTREG_CH0, 0x0000 );

            /* clear data RAM */    
            while (n--)
            {
                MWRITE_D16( llHdl->ma, (REG_START + count), 0x0000);
                count +=2;
            }
            
            MWRITE_D16(llHdl->ma, CONTREG_CH0, ((llHdl->options[0].baudRate) | 
            (llHdl->options[0].dataWidth << 2)));
            
            break;

        /*---------------------------------------------+
        |  set M47 transmission mode for all channels  |
        +---------------------------------------------*/
        case M47_TRANS_MODE:

            if(value < 0 || value > 1)
            {
                error = ERR_LL_ILL_PARAM;
                break;
            }
            
            llHdl->options[0].transMode =
            llHdl->options[1].transMode =
            llHdl->options[2].transMode =
            llHdl->options[3].transMode = (u_int16) value;
            
            /* stop transmission */
            MWRITE_D16( llHdl->ma, CONTREG_CH0, 0x0000 );

            /* clear data RAM */    
            while (n--)
            {
                MWRITE_D16( llHdl->ma, (REG_START + count), 0x0000);
                count +=2;
            }

            /* set transmission mode */
            MWRITE_D16(llHdl->ma, MODE_REV_CH0, (llHdl->options[0].transMode << 7));

            /* reinitialize transmission */
            MWRITE_D16(llHdl->ma, CONTREG_CH0, ((llHdl->options[0].baudRate) | 
            (llHdl->options[0].dataWidth << 2)));
            
            break;

        /*-----------------------------------------+
        |  set M47 baud rate for specific channel  |
        +-----------------------------------------*/
        case M47_BAUDRATE_CH:

            /* check module HW Revision */
            if( llHdl->moduleHwRev < HW_MAJOR_REV_2 ) {
                error = ERR_LL_ILL_FUNC;
                break;
            }

            /* check if channel in range */
            if(ch < 0 || ch > 3)
            {
                error = ERR_LL_ILL_CHAN;
                break;
            }
            
            /* check if value in range */
            if ( value < 0 || value > 3) {
                error = ERR_LL_ILL_PARAM;
                break;
            }
                
            llHdl->options[ch].baudRate = (u_int16) value;
            
            /* update Control Register */
            M47_UpdateControlRegs( llHdl );
            
            break;

        /*------------------------------------------+
        |  set M47 data width for specific channel  |
        +------------------------------------------*/
        case M47_DATA_WIDTH_CH:

            /* check module HW Revision */
            if( llHdl->moduleHwRev < HW_MAJOR_REV_2 ) {
                error = ERR_LL_ILL_FUNC;
                break;
            }

            /* check if channel in range */
            if(ch < 0 || ch > 3)
            {
                error = ERR_LL_ILL_CHAN;
                break;
            }

            if(value < 0 || value > 32)
            {
                error = ERR_LL_ILL_PARAM;
                break;
            }
            
            llHdl->options[ch].dataWidth = (u_int16) value;
            
            /* update control register */
            M47_UpdateControlRegs( llHdl );
            
            break;

        /*-------------------------------------------------+
        |  set M47 transmission mode for specific channel  |
        +-------------------------------------------------*/
        case M47_TRANS_MODE_CH:

            /* check module HW Revision */
            if( llHdl->moduleHwRev < HW_MAJOR_REV_2 ) {
                error = ERR_LL_ILL_FUNC;
                break;
            }

            /* check if channel in range */
            if(ch < 0 || ch > 3)
            {
                error = ERR_LL_ILL_CHAN;
                break;
            }

            if(value < 0 || value > 1)
            {
                error = ERR_LL_ILL_PARAM;
                break;
            }
            
            llHdl->options[ch].transMode = (u_int16) value;

            /* set transmission mode */
            MWRITE_D16(llHdl->ma, MODE_REV_CH0, (llHdl->options[0].transMode << 7));
            MWRITE_D16(llHdl->ma, MODE_REV_CH1, (llHdl->options[1].transMode << 7));
            MWRITE_D16(llHdl->ma, MODE_REV_CH2, (llHdl->options[2].transMode << 7));
            MWRITE_D16(llHdl->ma, MODE_REV_CH3, (llHdl->options[3].transMode << 7));

            DBGWRT_3((DBH, "LL - M47_SetStat: MODE_REV_CH0 = 0x%04x\n",
                     MREAD_D16(llHdl->ma, MODE_REV_CH0)));

            DBGWRT_3((DBH, "LL - M47_SetStat: MODE_REV_CH1 = 0x%04x\n",
                     MREAD_D16(llHdl->ma, MODE_REV_CH1)));

            DBGWRT_3((DBH, "LL - M47_SetStat: MODE_REV_CH2 = 0x%04x\n",
                     MREAD_D16(llHdl->ma, MODE_REV_CH2)));

            DBGWRT_3((DBH, "LL - M47_SetStat: MODE_REV_CH3 = 0x%04x\n",
                     MREAD_D16(llHdl->ma, MODE_REV_CH3)));
                           
            /* update Control Register */
            M47_UpdateControlRegs( llHdl );
            
            break;

        /*--------------------------+
        |  (unknown)                |
        +--------------------------*/
        default:
            error = ERR_LL_UNK_CODE;
    }

    return(error);
}

/****************************** M47_GetStat **********************************
 *
 *  Description:  Get the driver status
 *
 *                The following status codes are supported:
 *
 *                Code                 Description                 Values
 *                -------------------  --------------------------  ----------
 *                M_LL_DEBUG_LEVEL     driver debug level          see dbg.h
 *                M_LL_CH_NUMBER       number of channels          0..4
 *                M_LL_CH_DIR          direction of curr. chan.    M_CH_???
 *                M_LL_CH_LEN          length of curr. ch. [bits]  1..max
 *                M_LL_CH_TYP          description of curr. chan.  M_CH_???
 *                M_LL_IRQ_COUNT       interrupt counter           0..max
 *                M_LL_ID_CHECK        ID is checked               0..1
 *                M_LL_ID_SIZE         EEPROM size [bytes]         128
 *                M_LL_BLK_ID_DATA     EEPROM raw data             -
 *                M_MK_BLK_REV_ID      ident function table ptr    -
 *                M47_CHECK_CONNECT    check sensor connection     see below
 *                M47_BAUDRATE         baudrate for transmission   0..3 
 *                M47_DATA_WIDTH       width of data word          0..32
 *                M47_TRANS_MODE       transmission mode           0 (Gray) 
 *                                                                 1 (binary)
 *                M47_PLD_REV          PLD revision number         0..max
 *
 *
 *                M47_CHECK_CONNECT checks the sensor connection:
 *                    Bit      7..4        3..0
 *                        +------------------------+ 
 *                        |     RES    |TD|TC|TB|TA|
 *                        +------------------------+
 *                    RES: reserved
 *                    TD = 1: Channel D is transferring data
 *                    TC = 1: Channel C is transferring data
 *                    TB = 1: Channel B is transferring data
 *                    TA = 1: Channel A is transferring data
 *
 *                M47_BAUDRATE gets the baud rate for transmission:
 *                    0 = 500 kbaud
 *                    1 = 250 kbaud
 *                    2 = 125 kbaud
 *                    3 = 62.5 kbaud
 *                   (see m47_drv.h)
 *
 *                M47_HW_REV           HW Revision of module       0..max  
 *
 *                The following codes are only valid in conjunction with M47
 *                HW revision 2.0 or higher!
 *
 *                M47_BAUDRATE_CH      baudrate for specific CH    see 
 *                                                                 M47_BAUDRATE
 *                M47_DATA_WIDTH_CH    data width for specific CH  0..32
 *                M47_TRANS_MODE_CH    trans. mode for spceific CH 0 (Gray) 
 *                                                                 1 (binary)
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl             low-level handle
 *                code              status code
 *                ch                current channel
 *                value32_or_64P    pointer to block data structure (M_SG_BLOCK)(*) 
 *                                  (*) = for block status codes
 *  Output.....:  value32_or_64P    data pointer or
 *                                  pointer to block data structure (M_SG_BLOCK)(*) 
 *                return            success (0) or error code
 *                                  (*) = for block status codes
 *  Globals....:  ---
 ****************************************************************************/
static int32 M47_GetStat(
    LL_HANDLE *llHdl,
    int32  code,
    int32  ch,
    INT32_OR_64 *value32_or_64P
)
{
    int32       *valueP     = (int32*)value32_or_64P;       /* pointer to 32bit value  */
    INT32_OR_64 *value64P   = value32_or_64P;               /* stores 32/64bit pointer  */
    M_SG_BLOCK  *blk        = (M_SG_BLOCK*)value32_or_64P;  /* stores block struct pointer */

    int32 error = ERR_SUCCESS;

    DBGWRT_1((DBH, "LL - M47_GetStat: ch=%d code=0x%04x\n",
              ch,code));

    switch(code)
    {
        /*--------------------------+
        |  debug level              |
        +--------------------------*/
        case M_LL_DEBUG_LEVEL:
            *valueP = llHdl->dbgLevel;
            break;
        /*--------------------------+
        |  number of channels       |
        +--------------------------*/
        case M_LL_CH_NUMBER:
            *valueP = CH_NUMBER;
            break;
        /*--------------------------+
        |  channel direction        |
        +--------------------------*/
        case M_LL_CH_DIR:
            *valueP = M_CH_IN;
            break;
        /*--------------------------+
        |  channel length [bits]    |
        +--------------------------*/
        case M_LL_CH_LEN:
            *valueP = 32;
            break;
        /*--------------------------+
        |  channel type info        |
        +--------------------------*/
        case M_LL_CH_TYP:
            *valueP = M_CH_SERIAL;
            break;
        /*--------------------------+
        |  irq counter              |
        +--------------------------*/
        case M_LL_IRQ_COUNT:
            *valueP = llHdl->irqCount;
            break;
        /*--------------------------+
        |  ID PROM check enabled    |
        +--------------------------*/
        case M_LL_ID_CHECK:
            *valueP = llHdl->idCheck;
            break;
        /*--------------------------+
        |   ID PROM size            |
        +--------------------------*/
        case M_LL_ID_SIZE:
            *valueP = MOD_ID_SIZE;
            break;
        /*--------------------------+
        |   ID PROM data            |
        +--------------------------*/
        case M_LL_BLK_ID_DATA:
        {
            u_int8 n;
            u_int16 *dataP = (u_int16*)blk->data;

            if (blk->size < MOD_ID_SIZE)        /* check buf size */
                return(ERR_LL_USERBUF);

            for (n=0; n<MOD_ID_SIZE/2; n++)        /* read MOD_ID_SIZE/2 words */
                *dataP++ = (u_int16) m_read((U_INT32_OR_64)llHdl->ma, n);

            break;
        }
        /*--------------------------+
        |   ident table pointer     |
        |   (treat as non-block!)   |
        +--------------------------*/
        case M_MK_BLK_REV_ID:
           *value64P = (INT32_OR_64)&llHdl->idFuncTbl;
           break;


        /*--------------------------+
        |  get M47 baud rate        |
        +--------------------------*/
        case M47_BAUDRATE:
            
            *valueP = (int32) llHdl->options[0].baudRate;
            
            break;

        /*--------------------------+
        |  get M47 data width       |
        +--------------------------*/
        case M47_DATA_WIDTH:

            *valueP = (int32) llHdl->options[0].dataWidth;

            break;

        /*---------------------------+
        |  get M47 transmission mode |
        +---------------------------*/
        case M47_TRANS_MODE:
            
            *valueP = (int32) llHdl->options[0].transMode;
            
            break;

        /*-------------------------------------+
        |  get M47 baud rate for specific CH   |
        +-------------------------------------*/
        case M47_BAUDRATE_CH:

            /* check module HW Revision */
            if( llHdl->moduleHwRev < HW_MAJOR_REV_2 ) {
                error = ERR_LL_ILL_FUNC;
                break;
            }

            /* check if channel in range */
            if(ch < 0 || ch > 3)
            {
                error = ERR_LL_ILL_CHAN;
                break;
            }
                        
            *valueP = (int32) llHdl->options[ch].baudRate;
            
            break;

        /*--------------------------------------+
        |  get M47 data width for specific CH   |
        +--------------------------------------*/
        case M47_DATA_WIDTH_CH:

            /* check module HW Revision */
            if( llHdl->moduleHwRev < HW_MAJOR_REV_2 ) {
                error = ERR_LL_ILL_FUNC;
                break;
            }

            /* check if channel in range */
            if(ch < 0 || ch > 3)
            {
                error = ERR_LL_ILL_CHAN;
                break;
            }

            *valueP = (int32) llHdl->options[ch].dataWidth;

            break;

        /*---------------------------------------------+
        |  get M47 transmission mode for specific CH   |
        +---------------------------------------------*/
        case M47_TRANS_MODE_CH:

            /* check module HW Revision */
            if( llHdl->moduleHwRev < HW_MAJOR_REV_2 ) {
                error = ERR_LL_ILL_FUNC;
                break;
            }

            /* check if channel in range */
            if(ch < 0 || ch > 3)
            {
                error = ERR_LL_ILL_CHAN;
                break;
            }
            
            *valueP = (int32) llHdl->options[ch].transMode;
            
            break;

        /*---------------------------+
        |  get M47 PLD_Rev           |
        +---------------------------*/
        case M47_PLD_REV:
            
            *valueP = (int32) ((MREAD_D16( llHdl->ma, MODE_REV_CH0)) & 0x000f);
            
            break;

        /*-----------------------+
        |  get M47 HW Revision   |
        +-----------------------*/
        case M47_HW_REV:
            
            *valueP = (int32) llHdl->moduleHwRev;
            
            break;


        /*--------------------------+
        |  get M47 sensor connected |
        +--------------------------*/
        
        case M47_CHECK_CONNECT:
        
        MWRITE_D16(llHdl->ma, STATUS_REG, 0x0000);
        
        /* Delay for > 2 transmission cycles here 4 msec */
        OSS_Delay (llHdl->osHdl, 4);  

        *valueP = (int32) ((MREAD_D16(llHdl->ma, STATUS_REG))  & 0x000f);
        
        break;
        
            

        /*--------------------------+
        |  (unknown)                |
        +--------------------------*/
        default:
            error = ERR_LL_UNK_CODE;
    }

    return(error);
}

/******************************* M47_BlockRead *******************************
 *
 *  Description:  Read a data block (values of channels 0..3) from the device.
 *                The variable buf must be a pointer to unsigned long (u_int32)
 *                array with space for 4 values. The location of data in the
 *                buffer is shown below.
 *
 *                    buf[0]      buf[1]       buf[2]       buf[3]
 *                +-------------------------------------------------+
 *                |   value   |    value   |    value   |   value   |
 *                | channel 0 |  channel 1 |  channel 2 | channel 3 |
 *                +-------------------------------------------------+
 *---------------------------------------------------------------------------
 *  Input......:  llHdl        low-level handle
 *                ch           current channel
 *                buf          data buffer
 *                size         data buffer size
 *  Output.....:  nbrRdBytesP  number of read bytes
 *                return       success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M47_BlockRead(
     LL_HANDLE *llHdl,
     int32     ch,
     void      *buf,
     int32     size,
     int32     *nbrRdBytesP
)
{
    int32 i;
    u_int32* bufPointer;
    
    DBGWRT_1((DBH, "LL - M47_BlockRead: ch=%d, size=%d\n",ch,size));
    
    bufPointer = (u_int32*) buf;

    if (size < (4 * sizeof(u_int32)))
    {
        *nbrRdBytesP = 0;
        return (ERR_LL_USERBUF);
    }
    
    for ( i = 0; i < 4; i++)
    {
        M47_Read (llHdl, i, (int32*)bufPointer++);
    }

    /* return number of read bytes */
    *nbrRdBytesP = 16;

    return(ERR_SUCCESS);
}

/****************************** M47_BlockWrite *******************************
 *
 *  Description:  Do nothing
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl        low-level handle
 *                ch           current channel
 *                buf          data buffer
 *                size         data buffer size
 *  Output.....:  nbrWrBytesP  number of written bytes
 *                return       error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M47_BlockWrite(
     LL_HANDLE *llHdl,
     int32     ch,
     void      *buf,
     int32     size,
     int32     *nbrWrBytesP
)
{
    DBGWRT_1((DBH, "LL - M47_BlockWrite: ch=%d, size=%d\n",ch,size));

    /* return number of written bytes */
    *nbrWrBytesP = 0;  /* function not implemented */

    return(ERR_LL_ILL_FUNC);
}


/****************************** M47_Irq *************************************
 *
 *  Description:  Interrupt service routine
 *
 *                Unused - the module supports no interrupt.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl    ll handle
 *  Output.....:  return   LL_IRQ_DEVICE    irq caused from device
 *                         LL_IRQ_DEV_NOT   irq not caused from device
 *                         LL_IRQ_UNKNOWN   unknown
 *  Globals....:  ---
 ****************************************************************************/
static int32 M47_Irq(
   LL_HANDLE *llHdl
)
{
    IDBGWRT_1((DBH, ">>> M47_Irq:\n"));

    /* not my interrupt */
    return(LL_IRQ_DEV_NOT);
}

/****************************** M47_Info ************************************
 *
 *  Description:  Get information about hardware and driver requirements
 *
 *                The following info codes are supported:
 *
 *                Code                      Description
 *                ------------------------  -----------------------------
 *                LL_INFO_HW_CHARACTER      hardware characteristics
 *                LL_INFO_ADDRSPACE_COUNT   nr of required address spaces
 *                LL_INFO_ADDRSPACE         address space information
 *                LL_INFO_IRQ               interrupt required
 *                LL_INFO_LOCKMODE          process lock mode required
 *
 *                The LL_INFO_HW_CHARACTER code returns all address and 
 *                data modes (ORed) which are supported by the hardware
 *                (MDIS_MAxx, MDIS_MDxx).
 *
 *                The LL_INFO_ADDRSPACE_COUNT code returns the number
 *                of address spaces used by the driver.
 *
 *                The LL_INFO_ADDRSPACE code returns information about one
 *                specific address space (MDIS_MAxx, MDIS_MDxx). The returned 
 *                data mode represents the widest hardware access used by 
 *                the driver.
 *
 *                The LL_INFO_IRQ code returns whether the driver supports an
 *                interrupt routine (TRUE or FALSE).
 *
 *                The LL_INFO_LOCKMODE code returns which process locking
 *                mode the driver needs (LL_LOCK_xxx).
 *---------------------------------------------------------------------------
 *  Input......:  infoType      info code
 *                ...           argument(s)
 *  Output.....:  return        success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M47_Info(
   int32  infoType,
   ...
)
{
    int32   error = ERR_SUCCESS;
    va_list argptr;

    va_start(argptr, infoType );

    switch(infoType) {
        /*-------------------------------+
        |  hardware characteristics      |
        |  (all addr/data modes ORed)   |
        +-------------------------------*/
        case LL_INFO_HW_CHARACTER:
        {
            u_int32 *addrModeP = va_arg(argptr, u_int32*);
            u_int32 *dataModeP = va_arg(argptr, u_int32*);

            *addrModeP = MDIS_MA08;
            *dataModeP = MDIS_MD08 | MDIS_MD16;
            break;
        }
        /*-------------------------------+
        |  nr of required address spaces |
        |  (total spaces used)           |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE_COUNT:
        {
            u_int32 *nbrOfAddrSpaceP = va_arg(argptr, u_int32*);

            *nbrOfAddrSpaceP = ADDRSPACE_COUNT;
            break;
        }
        /*-------------------------------+
        |  address space type            |
        |  (widest used data mode)       |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE:
        {
            u_int32 addrSpaceIndex = va_arg(argptr, u_int32);
            u_int32 *addrModeP = va_arg(argptr, u_int32*);
            u_int32 *dataModeP = va_arg(argptr, u_int32*);
            u_int32 *addrSizeP = va_arg(argptr, u_int32*);

            if (addrSpaceIndex >= ADDRSPACE_COUNT)
                error = ERR_LL_ILL_PARAM;
            else {
                *addrModeP = MDIS_MA08;
                *dataModeP = MDIS_MD16;
                *addrSizeP = ADDRSPACE_SIZE;
            }

            break;
        }
        /*-------------------------------+
        |   interrupt required           |
        +-------------------------------*/
        case LL_INFO_IRQ:
        {
            u_int32 *useIrqP = va_arg(argptr, u_int32*);

            *useIrqP = USE_IRQ;
            break;
        }
        /*-------------------------------+
        |   process lock mode            |
        +-------------------------------*/
        case LL_INFO_LOCKMODE:
        {
            u_int32 *lockModeP = va_arg(argptr, u_int32*);

            *lockModeP = LL_LOCK_CALL;
            break;
        }
        /*-------------------------------+
        |   (unknown)                    |
        +-------------------------------*/
        default:
          error = ERR_LL_ILL_PARAM;
    }

    va_end(argptr);
    return(error);
}

/*******************************  Ident  ************************************
 *
 *  Description:  Return ident string
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  return  pointer to ident string
 *  Globals....:  -
 ****************************************************************************/
static char* Ident( void )    /* nodoc */
{
    return( (char*) IdentString );
}

/********************************* Cleanup **********************************
 *
 *  Description: Close all handles, free memory and return error code
 *                 NOTE: The low-level handle is invalid after this function is
 *                     called.
 *
 *---------------------------------------------------------------------------
 *  Input......: llHdl      low-level handle
 *               retCode    return value
 *  Output.....: return     retCode
 *  Globals....: -
 ****************************************************************************/
static int32 Cleanup(
   LL_HANDLE    *llHdl,
   int32        retCode     /* nodoc */
)
{
    /*------------------------------+
    |  close handles                |
    +------------------------------*/
    /* clean up desc */
    if (llHdl->descHdl)
        DESC_Exit(&llHdl->descHdl);

    /* clean up debug */
    DBGEXIT((&DBH));

    /*------------------------------+
    |  free memory                  |
    +------------------------------*/
    /* free my handle */
    OSS_MemFree(llHdl->osHdl, (int8*)llHdl, llHdl->memAlloc);

    /*------------------------------+
    |  return error code            |
    +------------------------------*/
    return(retCode);
}

/*************************  M47_FlexDataIdent  ******************************
 *
 *  Description:  Gets the pointer to ident string.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *
 *  Output.....:  return  pointer to ident string
 *
 *  Globals....:  -
 ****************************************************************************/
static char* M47_FlexDataIdent( void )  /* nodoc */
{
    return( (char*)M47_FlexIdent );
}

/*************************  M47_UpdateControlRegs  ****************************
 *
 *  Description:  Update control registers for all channels.
 *                After setting the registers this function waits for 1600 us
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl     low-level handle
 *
 *  Output.....:  -
 *
 *  Globals....:  -
 ****************************************************************************/
static void M47_UpdateControlRegs( LL_HANDLE *llHdl ) /* nodoc */
{
    
    MWRITE_D16(llHdl->ma, CONTREG_CH0, ((llHdl->options[0].baudRate) | 
    (llHdl->options[0].dataWidth << 2)));

    MWRITE_D16(llHdl->ma, CONTREG_CH1, ((llHdl->options[1].baudRate) | 
    (llHdl->options[1].dataWidth << 2)));
    
    MWRITE_D16(llHdl->ma, CONTREG_CH2, ((llHdl->options[2].baudRate) | 
    (llHdl->options[2].dataWidth << 2)));

    MWRITE_D16(llHdl->ma, CONTREG_CH3, ((llHdl->options[3].baudRate) | 
    (llHdl->options[3].dataWidth << 2)));    

    DBGWRT_3((DBH, "LL - M47_UpdateControlRegs: CONTREG_CH0 = 0x%04x\n",
                   MREAD_D16(llHdl->ma, CONTREG_CH0)));

    DBGWRT_3((DBH, "LL - M47_UpdateControlRegs: CONTREG_CH1 = 0x%04x\n",
                   MREAD_D16(llHdl->ma, CONTREG_CH1)));

    DBGWRT_3((DBH, "LL - M47_UpdateControlRegs: CONTREG_CH2 = 0x%04x\n",
                   MREAD_D16(llHdl->ma, CONTREG_CH2)));

    DBGWRT_3((DBH, "LL - M47_UpdateControlRegs: CONTREG_CH3 = 0x%04x\n",
                   MREAD_D16(llHdl->ma, CONTREG_CH3)));

}

