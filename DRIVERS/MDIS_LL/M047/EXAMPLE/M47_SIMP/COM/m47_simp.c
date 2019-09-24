/****************************************************************************
 ************                                                    ************
 ************                   M47_SIMP                         ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: ag
 *
 *  Description: Simple example program for the M47 driver
 *
 *               Reads value from SSI device 
 *                      
 *     Required: libraries: mdis_api
 *     Switches: -
 *
 *
 *---------------------------------------------------------------------------
 * Copyright 2002-2019, MEN Mikro Elektronik GmbH
 ****************************************************************************/


#include <MEN/men_typs.h>

#include <stdio.h>
#include <string.h>

#include <MEN/usr_oss.h>
#include <MEN/mdis_api.h>
#include <MEN/mdis_err.h>
#include <MEN/m47_drv.h>

static const char IdentString[]=MENT_XSTR(MAK_REVISION);

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define M47_MAX_CH   4

/*--------------------------------------+
|   TYPDEFS                             |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static int _m47_simple( char *devName, int32 ch );
static void PrintError(char *info);


/********************************* main *************************************
 *
 *  Description: Program main function
 *			   
 *---------------------------------------------------------------------------
 *  Input......: argc,argv	argument counter, data ..
 *  Output.....: return	    success (0) or error (1)
 *  Globals....: -
 ****************************************************************************/

int main(int argc, char *argv[])
{
	u_int32 ch;
	
	if (argc < 2 || strcmp(argv[1],"-?")==0) {
		printf("Syntax: m47_simp <device>\n");
		printf("Function: M47 example for reading from a channel\n");
		printf("Option:\n");
		printf("    device       device name\n");
		printf("\n");
		printf("%s\n", IdentString );
		printf("Build %s %s\n", __DATE__, __TIME__ );
		printf("\n");
		return(1);
	}
	
	ch = 0; /* Read form channel 0 */

	printf("\nM47-Simple program:\n Device name: %s\n Channel: %01ld\n", argv[1], ch);

	_m47_simple( argv[1],  ch);
	return (0);
}

/******************************* _m47_simple *********************************
 *
 *  Description:  Open the device and read from channel.
 *                Shows the read values.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  devName device name in the system e.g. "/m47/0"
 *                ch      channel to read from
 *  Output.....:  return  0 => Ok or 1 => Error
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
static int _m47_simple( char *devName, int32 ch )
{
	MDIS_PATH path;
	int32 chan; 
	u_int32 currVal;
	char *device;
	int i;
	u_int32 dummy[M47_MAX_CH];
	device = devName;
	chan = ch;


	/*--------------------+
	|  open path          |
	+--------------------*/
	printf("\nM_open");
	if ((path = M_open(device)) < 0) {
		PrintError("open");
		return(1);
	}

	/*------------------------+
	|  module configuration   |
	+------------------------*/
	printf("\nconfiguration - M_setstat");

	/* channel number */
	if ((M_setstat(path, M_MK_CH_CURRENT, chan)) < 0) {
		PrintError("setstat M_MK_CH_CURRENT");
		goto abort;
	}

	/* set baudrate for SSI device to 250 kbits/s */
	if (M_setstat(path, M47_BAUDRATE, M47_BAUD_250) < 0) {
		PrintError("setstat M47_BAUDRATE");
		goto abort;
	}

	/* set data width of SSI sensor to 15 */
	if (M_setstat(path, M47_DATA_WIDTH, 15) < 0) {
		PrintError("setstat M47_DATA_WIDTH");
		goto abort;
	}
	
	/* set transmission mode to gray-encoding */
	if (M_setstat(path, M47_TRANS_MODE, M47_TRANS_MODE_GRAY) < 0) {
		PrintError("setstat M47_TRANS_MODE");
		goto abort;
	}
	
	/* wait for configuration to take effect */
	UOS_MikroDelay( 1600 );
	
	/* perform dummy read from all channels to initialize buffer register */
	if (M_getblock( path, (u_int8*) dummy, M47_MAX_CH * sizeof (u_int32)) < 0){
		PrintError("getblock");
		goto abort;
	}
	
	/*------------------------------------+
	|  read value from SSI input channel  |
	+-------------------------------------*/
	printf("\nread value");
	
	/* read value */
	for ( i = 0; i < 1; i++)
	{
		if (M_read(path, (int32*)&currVal) < 0) {
			PrintError("read");
			goto abort;
		}
	}
	/* print read values with unvalid part masked */
	/* here only the first 15 bits (= data width of SSI sensor) are valid */ 
	printf("\n\nRead value = %08lX\n", (currVal & 0x00007fff));
	
	/*--------------------+
	|  close path         |
	+--------------------*/
	
	printf("\nM_close\n");
	
	if ( M_close (path) < 0) {
		PrintError("close");
		goto abort;
	}
	
	return(0);

	/*--------------------+
	|  clean-up            |
	+--------------------*/
	abort:
	if (M_close(path) < 0)
		PrintError("close");

	return(1);
}


/********************************* PrintError *******************************
 *
 *  Description: Print MDIS error message
 *			   
 *---------------------------------------------------------------------------
 *  Input......: info	info string
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void PrintError(char *info)
{
	printf("*** can't %s: %s\n", info, M_errstring(UOS_ErrnoGet()));
}


