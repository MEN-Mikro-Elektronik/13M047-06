/****************************************************************************
 ************                                                    ************
 ************                   M47_TEST                         ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: ag
 *
 *  Description: Test program for the M47 driver
 *
 *               Reads values form SSI-device and displays them cyclic
 *                      
 *     Required: libraries: mdis_api
 *     Switches: -
 *
 *---------------------------------------------------------------------------
 * Copyright 1999-2019, MEN Mikro Elektronik GmbH
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
#include <string.h>
#include <MEN/men_typs.h>
#include <MEN/m47_drv.h>
#include <MEN/usr_oss.h>
#include <MEN/mdis_api.h> 
#include <MEN/m47_drv.h>   /* M47 driver header file */

#define M47_HW_REV_2 0x0200

static const char IdentString[]=MENT_XSTR(MAK_REVISION);

static int _m47_test (char* devName);
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
	if (argc < 2 || strcmp(argv[1],"-?")==0) {
		printf("Syntax: m47_test <device>\n");
		printf("Function: M47 test for read/blockread from all channels\n");
		printf("Option:\n");
		printf("    device       device name\n");
		printf("\n");
		printf("%s\n", IdentString );
		printf("Build %s %s\n", __DATE__, __TIME__ );
		printf("\n");
		return(1);
	}
	
	printf("M47 - Test %s\n", IdentString );
	return( _m47_test(argv[1]) );
}/*main*/


static int _m47_test (char* devName)
{
	u_int32 error;
	u_int32 chan;
	MDIS_PATH fd;
	int32 connect, data;
	u_int32 singlBuf1[4];
	u_int32 singlBuf2[4];
	u_int32 blkBuf[4];
	u_int32 counter;
	int32 hwRev;
	int16 i;
	u_int32 baudrate[] = { M47_BAUD_500, 
	                       M47_BAUD_125, 
	                       M47_BAUD_62_5, 
	                       M47_BAUD_250 };
	u_int32 dataWidth[] = { 15, 10, 32, 20 };
	u_int32 transMode[] = { M47_TRANS_MODE_GRAY, 
	                        M47_TRANS_MODE_BIN, 
	                        M47_TRANS_MODE_BIN, 
	                        M47_TRANS_MODE_GRAY };
	                        
	fd = M_open( devName );
	if (fd < 0)
		goto ERR;
	
	printf("\n\n\n");

	/* Get HW-Rev. */
	if (M_getstat(fd, M47_HW_REV, &hwRev) < 0)
		goto ERR;


	/*----------------------------+
    |  Check all Get/Set codes    |
    +----------------------------*/
	
	/*-------------------------------------+
    |  Get/Set Baudrate for all channels   |
    +-------------------------------------*/
	if (M_setstat(fd, M47_BAUDRATE, M47_BAUD_500) < 0)
		goto ERR;
	
	if (M_getstat(fd, M47_BAUDRATE, &data) < 0)
		goto ERR;
	printf("Baudrate set to %08lX\n", data);
	
	if (M_setstat(fd, M47_BAUDRATE, M47_BAUD_250) < 0)
		goto ERR;

	if (M_getstat(fd, M47_BAUDRATE, &data) < 0)
		goto ERR;
	printf("Baudrate set to %08lX\n", data);

	if (M_setstat(fd, M47_BAUDRATE, M47_BAUD_125) < 0)
		goto ERR;

	if (M_getstat(fd, M47_BAUDRATE, &data) < 0)
		goto ERR;
	printf("Baudrate set to %08lX\n", data);

	if (M_setstat(fd, M47_BAUDRATE, M47_BAUD_62_5) < 0)
		goto ERR;
	
	if (M_getstat(fd, M47_BAUDRATE, &data) < 0)
		goto ERR;
	printf("Baudrate set to %08lX\n", data);
	
	/*---------------------------------------+
    |  Get/Set Data width for all channels   |
    +---------------------------------------*/
	if (M_setstat(fd, M47_DATA_WIDTH, 17) < 0)
		goto ERR;
	
	if (M_getstat(fd, M47_DATA_WIDTH, &data) < 0)
		goto ERR;
	printf("Data width set to %ld\n", data);
	
	/*---------------------------------------+
    |  Get/Set Transmode for all channels    |
    +---------------------------------------*/
	if (M_getstat(fd, M47_TRANS_MODE, &data) < 0)
		goto ERR;
	printf("Transmode:  %08lX\n", data);
	
	if (M_setstat(fd, M47_TRANS_MODE, M47_TRANS_MODE_BIN) < 0)
		goto ERR;

	if (M_getstat(fd, M47_TRANS_MODE, &data) < 0)
		goto ERR;
	printf("Transmode set to %08lX\n", data);
	
		
    /* Set conifguration values needed */ 
	if (M_setstat(fd, M47_BAUDRATE, M47_BAUD_62_5) < 0)
		goto ERR;

	if (M_getstat(fd, M47_BAUDRATE, &data) < 0)
		goto ERR;
	printf("Baudrate set to %08lX\n", data);

	if (M_setstat(fd, M47_DATA_WIDTH, 32) < 0)
		goto ERR;

	if (M_getstat(fd, M47_DATA_WIDTH, &data) < 0)
		goto ERR;
	printf("Data width set to %ld\n", data);
	
	
	if (M_setstat(fd, M47_TRANS_MODE, M47_TRANS_MODE_GRAY) < 0)
		goto ERR;
	
	
	if (M_getstat(fd, M47_TRANS_MODE, &data) < 0)
		goto ERR;
	printf("Transmode set to %08lX\n", data);


	/* Get PLD_Rev */
	if (M_getstat(fd, M47_PLD_REV, &data) < 0)
		goto ERR;
	printf("\nPLD_Rev: %08lX\n\n", data);

	/* Get Module HW Revision */
	if (M_getstat(fd, M47_HW_REV, &data) < 0)
		goto ERR;
	printf("\nHW_Rev: %08lX\n\n", data);

	/* Get Check connection */
	if (M_getstat(fd, M47_CHECK_CONNECT, &connect) < 0)
		goto ERR;
	printf("\nSensor connected: %08lX\n\n", connect);


	printf("\n\n");
	printf("============= Switch/Values ============\n");
	printf("0 => all zero\n");
	printf("R => 196130F2 9ECF0DE6 CF0DE69E F2196130\n");
	printf("1 => 196130F2 00000000 00000000 00000000\n");
	printf("2 => 00000000 196130F2 00000000 00000000\n");
	printf("3 => 00000000 00000000 196130F2 00000000\n");
	printf("4 => 00000000 00000000 00000000 196130F2\n");
	printf("press <ESC> to finish\n\n");
	
	
	counter=1;
	while(counter)
	{
	
		/*(buffer[x] & 0x00007fff) = mask for 15 bit data-width sensors */
	 	
		/* Read each channel */
		if( counter & 1 )
		{
			for( chan = 0; chan < 4; chan++ )
			{
				M_setstat(fd, M_MK_CH_CURRENT, chan);
				M_read(fd, (int32*)&singlBuf1[chan]);
				M_read(fd, (int32*)&singlBuf2[chan]);
			}/*for*/
		}
		else
		{
			for( chan = 3; chan < 4 ; chan-- )
			{
				M_setstat(fd, M_MK_CH_CURRENT, chan);
				M_read(fd, (int32*)&singlBuf1[chan]);
				M_read(fd, (int32*)&singlBuf2[chan]);
			}/*for*/
		}/*if*/


		/* BlockRead each channel */
		if (M_getblock( fd, (u_int8*) blkBuf, 4 * sizeof (u_int32)) < 0)
			goto ERR;

		/* check for equal values of each channel */
		for( chan = 0; chan < 4; chan++ )
		{
			if( singlBuf1[chan] != singlBuf2[chan] )
			{
				printf("*** M_read     channel %ld pattern %08lX != %08lX\n", chan, singlBuf1[chan], singlBuf2[chan] ); 
			}/*if*/
			if( singlBuf1[chan] != blkBuf[chan] )
			{
				printf("*** M_getblock channel %ld pattern %08lX != %08lX\n", chan, singlBuf1[chan], blkBuf[chan] ); 
			}/*if*/
		}/*for*/

		if( !(counter%10) )
		{
			printf("     %08lX %08lX %08lX %08lX\r", blkBuf[0], blkBuf[1], blkBuf[2], blkBuf[3]); 
			if( UOS_KeyPressed() == 27 /*ESC*/ )
				break;
			UOS_Delay( 1 );
		}/*if*/
	
		counter++;
	}/*while*/

	if( hwRev >= M47_HW_REV_2 ) {
		printf("\nChecking Get/Set States for HW-Revision %04x\n", hwRev );
		/*------------------------------------------+
	    |  set parameters for channels separately   |
	    +------------------------------------------*/
		for( i = 0; i < 4; i++ ) {
			if ((M_setstat(fd, M_MK_CH_CURRENT, i)) < 0) {
				PrintError("setstat M_MK_CH_CURRENT");
				goto ERR;
			}
			if (M_setstat(fd, M47_DATA_WIDTH_CH, dataWidth[i] ) < 0) {
				PrintError("setstat M47_DATA_WIDTH_CH");
				goto ERR;
			}
			if (M_setstat(fd, M47_TRANS_MODE_CH, transMode[i] ) < 0) {
				PrintError("setstat M47_TRANS_MODE_CH");
				goto ERR;
			}
			if (M_setstat(fd, M47_BAUDRATE_CH, baudrate[i] ) < 0) {
				PrintError("setstat M47_BAUDRATE_CH");
				goto ERR;
			}
		} /* for */
	

		/*--------------------------------+
	    |  check parameters for channels  |
	    +--------------------------------*/
		for( i = 0; i < 4; i++ ) {
			if ((M_setstat(fd, M_MK_CH_CURRENT, i)) < 0) {
				PrintError("setstat M_MK_CH_CURRENT");
				goto ERR;
			}
		
			if (M_getstat(fd, M47_DATA_WIDTH_CH, &data) < 0) {
				PrintError("getstat M47_DATA_WIDTH_CH");
				goto ERR;
			}
			if( data != dataWidth[i] ) {
				printf("*** ERROR Data width CH %d should be %02ld is %02ld\n", 
				       i, dataWidth[i], data);
				goto ERR;
			}
			if (M_getstat(fd, M47_TRANS_MODE_CH, &data) < 0) {
				PrintError("getstat M47_TRANS_MODE_CH");
				goto ERR;
			}
			if( data != transMode[i] ) {
				printf("*** ERROR Transmission mode CH %d should be %02ld "
				       "is %02ld\n", i, transMode[i], data);
				goto ERR;
			}
		
			if (M_getstat(fd, M47_BAUDRATE_CH, &data) < 0) {
				PrintError("getstat M47_BAUDRATE_CH");
				goto ERR;
			}
			if( data != baudrate[i] ) {
				printf("*** ERROR Baudrate CH %d should be %02ld "
				       "is %02ld\n", i, baudrate[i], data);
				goto ERR;
			}
		} /* for */

	} /* if */


	printf("\n close path");
	M_close (fd);
	return( 0 );

ERR:
	error = UOS_ErrnoGet();
	printf("*** %s ***\n",M_errstring( error ) );
	if( fd > -1 )
	{
		printf("\n close path");
		M_close( fd );
	}/*if*/

	return( 1 );
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


