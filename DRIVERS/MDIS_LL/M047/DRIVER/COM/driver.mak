#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2002/03/20 09:32:14 $
#      $Revision: 1.3 $
#
#    Description: Makefile definitions for the M47 driver
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: driver.mak,v $
#   Revision 1.3  2002/03/20 09:32:14  Schmidt
#   unused mbuf library and header file removed
#
#   Revision 1.2  2001/07/23 14:32:37  ww
#   included m47_flex.h in list of dependency
#
#   Revision 1.1  1999/11/18 15:20:00  Gromann
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=m47

MAK_SWITCH=$(SW_PREFIX)MAC_MEM_MAPPED

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/desc$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/oss$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/id$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/dbg$(LIB_SUFFIX)	\


MAK_INCL=$(MEN_INC_DIR)/m47_drv.h	\
         $(MEN_INC_DIR)/men_typs.h	\
         $(MEN_INC_DIR)/oss.h		\
         $(MEN_INC_DIR)/mdis_err.h	\
         $(MEN_INC_DIR)/maccess.h	\
         $(MEN_INC_DIR)/desc.h		\
         $(MEN_INC_DIR)/mdis_api.h	\
         $(MEN_INC_DIR)/mdis_com.h	\
         $(MEN_INC_DIR)/modcom.h	\
         $(MEN_INC_DIR)/ll_defs.h	\
         $(MEN_INC_DIR)/ll_entry.h	\
         $(MEN_INC_DIR)/dbg.h		\
         $(MEN_MOD_DIR)/m47_flex.h

MAK_INP1=m47_drv$(INP_SUFFIX)
MAK_INP2=

MAK_INP=$(MAK_INP1) \
        $(MAK_INP2)

