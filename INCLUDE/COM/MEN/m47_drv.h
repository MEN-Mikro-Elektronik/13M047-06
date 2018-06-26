/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: m47_drv.h
 *
 *       Author: ag
 *        $Date: 2013/06/18 18:03:50 $
 *    $Revision: 2.6 $
 *
 *  Description: Header file for M47 driver
 *               - M47 specific status codes
 *               - M47 function prototypes
 *
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
 *               _LL_DRV_
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m47_drv.h,v $
 * Revision 2.6  2013/06/18 18:03:50  gv
 * R: Porting to MDIS
 * M: Changed according to MDIS Porting Guide 0.9
 *
 * Revision 2.5  2004/03/23 15:39:53  AGromann
 * added support for additional feature of HW Rev 2.0:
 * Baudrate, Data Width and Transmission Mode can be set channel specific
 *
 * Revision 2.4  2003/04/09 08:46:01  kp
 * support swapped variant
 *
 * Revision 2.3  1999/11/25 08:58:43  Gromann
 * Updated comments
 *
 * Revision 2.2  1999/11/18 15:20:16  Gromann
 * cosmetics
 *
 * Revision 2.1  1999/11/17 11:46:34  Gromann
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _M47_DRV_H
#define _M47_DRV_H

#ifdef __cplusplus
      extern "C" {
#endif


/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/* none */

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/* M47 specific status codes (STD) */			/* S,G: S=setstat, G=getstat */
#define M47_BAUDRATE           M_DEV_OF+0x00	/* G,S: Baudrate for data transmission */
#define M47_DATA_WIDTH         M_DEV_OF+0x01	/* G,S: Data width for transmission */
#define M47_TRANS_MODE         M_DEV_OF+0x02	/* G,S: Transmission mode (gray or binery) */
#define M47_CHECK_CONNECT      M_DEV_OF+0x03	/* G:   Check if sensor is conected */
#define M47_PLD_REV            M_DEV_OF+0x04    /* G:   Get PLD Revision */
#define M47_BAUDRATE_CH        M_DEV_OF+0x05	/* G,S: Baudrate for specific channel */
#define M47_DATA_WIDTH_CH      M_DEV_OF+0x06	/* G,S: Data width for specific channel */
#define M47_TRANS_MODE_CH      M_DEV_OF+0x07	/* G,S: Transmission mode (gray or binery) */
												/*      for specific channel */
#define M47_HW_REV             M_DEV_OF+0x08	/* G:   HW revision of module */

/* M47 specific value coding (STD) */			
#define M47_BAUD_500           0x0000			/* Baudrate 500 kbaud */
#define M47_BAUD_250           0x0001			/* Baudrate 250 kbaud */
#define M47_BAUD_125           0x0002			/* Baudrate 125 kbaud */
#define M47_BAUD_62_5          0x0003			/* Baudrate 62.5 kbaud */
#define M47_TRANS_MODE_GRAY    0x0000			/* Transmission mode gray-code */
#define M47_TRANS_MODE_BIN     0x0001			/* Transmission mode binery-code */


/* M47 specific status codes (BLK)	*/			/* S,G: S=setstat, G=getstat */
/*#define M47_BLK_XXX       M_DEV_BLK_OF+0x00 *//* G,S: xxx */

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
#ifdef _LL_DRV_
# ifndef _ONE_NAMESPACE_PER_DRIVER_
#  ifndef MAC_BYTESWAP
    extern void M47_GetEntry( LL_ENTRY* drvP );
#  else
    extern void M47_SW_GetEntry( LL_ENTRY* drvP );
#  endif /* MAC_BYTESWAP */
# endif /* _ONE_NAMESPACE_PER_DRIVER_ */
#endif /* _LL_DRV_ */

/*-----------------------------------------+
|  BACKWARD COMPATIBILITY TO MDIS4         |
+-----------------------------------------*/
#ifndef U_INT32_OR_64
 /* we have an MDIS4 men_types.h and mdis_api.h included */
 /* only 32bit compatibility needed!                     */
 #define INT32_OR_64  int32
 #define U_INT32_OR_64 u_int32
 typedef INT32_OR_64  MDIS_PATH;
#endif /* U_INT32_OR_64 */

#ifdef __cplusplus
      }
#endif

#endif /* _M47_DRV_H */

