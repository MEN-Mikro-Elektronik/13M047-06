#***************************  M a k e f i l e  *******************************
#
#         Author: Gromann
#          $Date: 2001/01/16 11:40:56 $
#      $Revision: 1.2 $
#
#    Description: Makefile definitions for the M47 example program
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.2  2001/01/16 11:40:56  Schmidt
#   usr_oss lib added
#
#   Revision 1.1  1999/11/18 15:20:06  Gromann
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=m47_simp

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX)	\
		 $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)

MAK_INCL=$(MEN_INC_DIR)/m47_drv.h	\
         $(MEN_INC_DIR)/men_typs.h	\
         $(MEN_INC_DIR)/mdis_api.h	\
         $(MEN_INC_DIR)/mdis_err.h	\
         $(MEN_INC_DIR)/usr_oss.h         

MAK_INP1=m47_simp$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)

