/****************************************************************************
 ************                                                    ************
 ************                   M47_TOOL                         ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: ag
 *
 *  Description: M47 tool program
 *
 *               This tool program first checks the HW-Revision of the M47.
 *               Depending on the module revision, the program asks for the   
 *               initialization parameters for all channels (up to HW-Rev. 1.5)
 *               or for separate parameter for each channel (HW-Rev. 2.0 or
 *               higher). Then it cyclically read the values of all channels 
 *               from SSI-device.
 *                      
 *     Required: libraries: mdis_api, usr_oss
 *     Switches: VXWORKS
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
 
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/mdis_api.h>
#include <MEN/mdis_err.h>
#include <MEN/m47_drv.h>

static const char IdentString[]=MENT_XSTR(MAK_REVISION);

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define M47_MAX_CH   4
#define M47_HW_REV_2 0x0200

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
static int _m47_tool( char *devName, int32 ch );
static void PrintError(char *info);
static int32 GetValue(int32 def);


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
		printf("Syntax: m47_tool <device>\n");
		printf("Function: M47 tool program for reading all channels\n");
		printf("Option:\n");
		printf("    device       device name\n");
		printf("\n");
		printf("%s\n", IdentString );
		printf("Build %s %s\n", __DATE__, __TIME__ );
		printf("\n");
		return(1);
	}
	
	ch = 0; /* Read form channel 0 */

	printf("\nM47-Tool program:\n Device name: %s\n", argv[1]);

	_m47_tool( argv[1],  ch);
	return (0);
}

/******************************* _m47_tool ***********************************
 *
 *  Description:  Opens the device and cyclically reads from all channels.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  devName device name in the system e.g. "/m47/0"
 *                ch      channel to read form
 *  Output.....:  return  0 => Ok or 1 => Error
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
static int _m47_tool( char *devName, int32 ch )
{
	MDIS_PATH path;
	int32   chan; 
	u_int32 currVal[M47_MAX_CH];
	char    *device;
	int i;
	int32   hwRev;
	int32   sensEncoding = 0;
	int32   dataWidth    = 32;
	int32   baudrate     = 3;
	u_int32 dummy[M47_MAX_CH];

	device = devName;
	chan = ch;
	(void) chan;
	printf("\n++++ M47_Tool ++++\n");

	/*--------------------+
	|  open path          |
	+--------------------*/
	if ((path = M_open(device)) < 0) {
		PrintError("open");
		return(1);
	}

	/*-------------------------------------+
	|  initialization of SSI-Transmission  |
	+-------------------------------------*/

	/* check module hardware revision */
	if (M_getstat(path, M47_HW_REV, &hwRev) < 0)
		goto abort;

	printf("\nM47 HW_Rev: %08lX\n\n", hwRev);

	if( hwRev < M47_HW_REV_2 ) {
		/* modules with HW revision up to 1.5 */
		printf("Your module only supports to set the moduel parameters to \n"
		       "the same values for all channels:\n\n");
		do {
			printf("Enter Sensor encoding mode (0 = gray 1 = binary) ");
			sensEncoding = GetValue( sensEncoding );
		} while( sensEncoding > 1 );

		do {
			printf("Enter Data Width (1 - 32 bit) ");
			dataWidth = GetValue( dataWidth );
		} while( dataWidth > 32 );

		do {
			printf("Enter Baudrate (0 = 500, 1 = 250, 2 = 125, 3 = 62,5 kBaud) ");
			baudrate = GetValue( baudrate );
		} while( baudrate > 3 );

		/* set baudrate for SSI device */
		if (M_setstat(path, M47_BAUDRATE, baudrate) < 0) {
			PrintError("setstat M47_BAUDRATE");
			goto abort;
		}
	
		/* set data width of SSI sensor */
		if (M_setstat(path, M47_DATA_WIDTH, dataWidth) < 0) {
			PrintError("setstat M47_DATA_WIDTH");
			goto abort;
		}
		
		/* set transmission mode */
		if (M_setstat(path, M47_TRANS_MODE, sensEncoding) < 0) {
			PrintError("setstat M47_TRANS_MODE");
			goto abort;
		}

	} /* if */
	else {
		/* modules with HW revision 2.0 or higher */
		printf("Your module supports to set the moduel parameters separately\n"
		       "for each channel:\n\n");
		       
		for( i = 0; i < M47_MAX_CH; i++ ) {
			
			do {
				printf("Enter Sensor encoding mode CH%01d (0 = gray 1 = binary): ",
				       i );
				sensEncoding = GetValue( sensEncoding );
			} while( sensEncoding > 1 );
	
			do {
				printf("Enter Data Width CH%01d (1 - 32 bit): ", i);
				dataWidth = GetValue( dataWidth );
			} while( dataWidth > 32 );
	
			do {
				printf("Enter Baudrate CH%01d (0 = 500, 1 = 250, 2 = 125, "
				       "3 = 62,5 kBaud): ", i);
				baudrate = GetValue( baudrate );
			} while( baudrate > 3 );

			printf("\n");
				
			/* set current channel*/
			if ((M_setstat(path, M_MK_CH_CURRENT, i)) < 0) {
				PrintError("setstat M_MK_CH_CURRENT");
				goto abort;
			}
			/* set data width for current channel of SSI sensor */
			if (M_setstat(path, M47_DATA_WIDTH_CH, dataWidth) < 0) {
				PrintError("setstat M47_DATA_WIDTH_CH");
				goto abort;
			}
			/* set transmission mode of current channel */
			if (M_setstat(path, M47_TRANS_MODE_CH, sensEncoding) < 0) {
				PrintError("setstat M47_TRANS_MODE_CH");
				goto abort;
			}
			/* set baudrate for current channel of SSI device */
			if (M_setstat(path, M47_BAUDRATE_CH, baudrate) < 0) {
				PrintError("setstat M47_BAUDRATE_CH");
				goto abort;
			}
						
		} /* for */
		
	} /* else */

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
	printf("\nReading CH0 - CH3 cyclically\n");
	printf("\nPress ESC to Stop\n\n\n");

	
	while (UOS_KeyPressed() != 27 /* ESC */) {

		for (i = 0; i < 4; i++) {		

			/* channel number */
			if ((M_setstat(path, M_MK_CH_CURRENT, i)) < 0) {
				PrintError("setstat M_MK_CH_CURRENT");
				goto abort;
			}
			
			/* read value */
			if (M_read(path, (int32*)&currVal[i]) < 0) {
				PrintError("read");
				goto abort;
			}
	
		} /* for */

		/* print readen values */
		printf("CH0 = %08lX CH1 = %08lX CH2 = %08lX CH3 = %08lX", 
		currVal[0], currVal[1], currVal[2], currVal[3]);
		/* clear line */
		for( i = 0; i < 60; i++ )
			printf("\b");

	} /* while */

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
	|  cleanup            |
	+--------------------*/
	abort:
	if (M_close(path) < 0)
		PrintError("close");

	return(1);
}


/********************************* GetValue **********************************
 *
 *  Description: Read int32 value with default option
 *			   
 *---------------------------------------------------------------------------
 *  Input......: def	default value
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static int32 GetValue(int32 def)
{
	char buf[20];
	int32 val;

	printf("[%02ld]: ", def );
	if( !fgets( buf, sizeof(buf), stdin ) ) {
		PrintError("fgets()");
		return def;
	}

	if( buf[0] == '\n' )
		return def;

	val = atoi( buf );
	return val;
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

