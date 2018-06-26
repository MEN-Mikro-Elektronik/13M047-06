#***************************  M a k e f i l e  *******************************
#
#         Author: Gromann
#          $Date: 2004/03/23 15:39:49 $
#      $Revision: 1.1 $
#
#    Description: Makefile definitions for the M47 tool program
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.1  2004/03/23 15:39:49  AGromann
#   Initial Revision
#
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2003 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=m47_tool

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX)	\
		 $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)

MAK_INCL=$(MEN_INC_DIR)/m47_drv.h	\
         $(MEN_INC_DIR)/men_typs.h	\
         $(MEN_INC_DIR)/mdis_api.h	\
         $(MEN_INC_DIR)/mdis_err.h	\
         $(MEN_INC_DIR)/usr_oss.h         

MAK_INP1=m47_tool$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)

