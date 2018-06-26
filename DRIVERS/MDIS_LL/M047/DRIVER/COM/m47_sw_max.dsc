#************************** MDIS4 device descriptor *************************
#
# m47_sw_max.dsc: Descriptor for M47
# Automatically generated by mdiswiz 0.97a.003-linux-1 from 13m04706.xml
# 2004-08-24
#
#****************************************************************************

M47_SW_1 {

    # ------------------------------------------------------------------------
    #        general parameters (don't modify)
    # ------------------------------------------------------------------------
    DESC_TYPE = U_INT32 0x1
    HW_TYPE = STRING M47_sw

    # ------------------------------------------------------------------------
    #        reference to base board
    # ------------------------------------------------------------------------
    BOARD_NAME = STRING A12_1
    DEVICE_SLOT = U_INT32 0x0

    # ------------------------------------------------------------------------
    #        device parameters
    # ------------------------------------------------------------------------

    # Define wether M-Module ID-PROM is checked
    # 0 := disable -- ignore IDPROM
    # 1 := enable
    ID_CHECK = U_INT32 1

    # Baudrate and bits in data word. See user manual.
    M47_CONTROL = U_INT32 0x80

    # Transmission mode
    # 0x0 := Gray
    # 0x80 := binary
    M47_TRANSMODE = U_INT32 0x0

    # ------------------------------------------------------------------------
    #        debug levels (optional)
    #        this keys have only effect on debug drivers
    # ------------------------------------------------------------------------
    DEBUG_LEVEL = U_INT32 0xc0008000
    DEBUG_LEVEL_MK = U_INT32 0xc0008000
    DEBUG_LEVEL_OSS = U_INT32 0xc0008000
    DEBUG_LEVEL_DESC = U_INT32 0xc0008000
}
# EOF