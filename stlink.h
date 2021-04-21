
/* lsusb shows STLINK has 3 endpoints: bNumEndpoints 3, all 3 with bullk
 * transfer, 64 bytes max packet size.
 * Also the id's:
 *    idVendor           0x0483 STMicroelectronics
 *    idProduct          0x3748 ST-LINK/V2
 * bNumInterfaces          1
 * bInterfaceNumber        0
 *
 * libusb:
 * uint8_t libusb_endpoint_descriptor: bEndpointAddress
 * Bits 0:3 are the endpoint number. Bits 4:6 are reserved. Bit 7 indicates
 * direction:
 * LIBUSB_ENDPOINT_OUT: host-to-device.
 * LIBUSB_ENDPOINT_IN : device-to-host.
 */
#define STLINK_USB_ENDPOINT_IN1		0x81  //EP 1 IN
#define STLINK_USB_ENDPOINT_OUT2	0x02  //EP 2 IN
#define STLINK_USB_ENDPOINT_IN3		0x83  //EP 3 IN
#define STLINK_USB_VENDOR_ID		0x0483
#define STLINK_USB_PRODUCT_ID		0x3748

enum stlink_commands {
  STLINK_GET_VERSION	    = 0xF1,    //(null) -> x6
  STLINK_DEBUG_COMMAND      = 0xF2,
  STLINK_DFU_COMMAND        = 0xF3,
  STLINK_SWIM_COMMAND       = 0xF4,
  STLINK_GET_CURRENT_MODE   = 0xF5,
  STLINK_GET_TARGET_VOLTAGE = 0xF7
};

enum stlink_mode {
  STLINK_MODE_DFU           = 0x0000,  //Direct Firmware Update
  STLINK_MODE_MASS          = 0x0001,
  STLINK_MODE_DEBUG         = 0x0002,
  STLINK_MODE_SWIM          = 0x0300,
  STLINK_MODE_BOOTLOADER    = 0x0004,
  STLINK_MODE_NONE          = 0x0100
};

enum stlink_swim_commands {
  STLINK_SWIM_ENTER         = 0x00,
  STLINK_SWIM_EXIT          = 0x01,
  STLINK_SWIM_READ_CAP      = 0x02,
  STLINK_SWIM_SPEED         = 0x03,    //uint8_t (0=low|1=high)
  STLINK_SWIM_ENTER_SEQ     = 0x04,
  STLINK_SWIM_GEN_RST       = 0x05,
  STLINK_SWIM_RESET         = 0x06,
  STLINK_SWIM_NRES_LOW      = 0x07,
  STLINK_SWIM_NRES_HIGH     = 0x08,
  STLINK_SWIM_READSTATUS    = 0x09,
  STLINK_SWIM_WRITEMEM      = 0x0A,
  STLINK_SWIM_READMEM       = 0x0B,
  STLINK_SWIM_READBUF       = 0x0C
};

#define STLINK_DFU_EXIT			0x07

#define STM8_DM_CR1			0x7F96
#define STM8_DM_CR2			0x7F97
#define STM8_DM_CSR1			0x7F98
#define STM8_DM_CSR2			0x7F99

/* The below definitions are not used yet, as they are debugging commands not
 * implemented yet
 */
enum stlink_debug_commands {
    STLINK_DEBUG_ENTER_JTAG              = 0x00,
    STLINK_DEBUG_GETSTATUS               = 0x01,
    STLINK_DEBUG_FORCEDEBUG              = 0x02,
    STLINK_DEBUG_APIV1_RESETSYS          = 0x03,
    STLINK_DEBUG_APIV1_READALLREGS       = 0x04,
    STLINK_DEBUG_APIV1_READREG           = 0x05,
    STLINK_DEBUG_APIV1_WRITEREG          = 0x06,
    STLINK_DEBUG_READMEM_32BIT           = 0x07,
    STLINK_DEBUG_WRITEMEM_32BIT          = 0x08,
    STLINK_DEBUG_RUNCORE                 = 0x09,
    STLINK_DEBUG_STEPCORE                = 0x0a,
    STLINK_DEBUG_APIV1_SETFP             = 0x0b,
    STLINK_DEBUG_WRITEMEM_8BIT           = 0x0d,
    STLINK_DEBUG_APIV1_CLEARFP           = 0x0e,
    STLINK_DEBUG_APIV1_WRITEDEBUGREG     = 0x0f,
    STLINK_DEBUG_APIV1_ENTER             = 0x20,
    STLINK_DEBUG_EXIT                    = 0x21,
    STLINK_DEBUG_READCOREID              = 0x22,

    STLINK_DEBUG_APIV2_ENTER             = 0x30,
    STLINK_DEBUG_APIV2_READ_IDCODES      = 0x31,
    STLINK_DEBUG_APIV2_RESETSYS          = 0x32,
    STLINK_DEBUG_APIV2_READREG           = 0x33,
    STLINK_DEBUG_APIV2_WRITEREG          = 0x34,
    STLINK_DEBUG_APIV2_WRITEDEBUGREG     = 0x35,
    STLINK_DEBUG_APIV2_READDEBUGREG      = 0x36,

    STLINK_DEBUG_APIV2_READALLREGS       = 0x3A,
    STLINK_DEBUG_APIV2_GETLASTRWSTATUS   = 0x3B,
    STLINK_DEBUG_APIV2_DRIVE_NRST        = 0x3C,
    STLINK_DEBUG_APIV2_GETLASTRWSTATUS2  = 0x3E,

    STLINK_DEBUG_APIV2_START_TRACE_RX    = 0x40,
    STLINK_DEBUG_APIV2_STOP_TRACE_RX     = 0x41,
    STLINK_DEBUG_APIV2_GET_TRACE_NB      = 0x42,
    STLINK_DEBUG_APIV2_SWD_SET_FREQ      = 0x43,
    STLINK_DEBUG_APIV2_JTAG_SET_FREQ     = 0x44,
    STLINK_DEBUG_APIV2_READ_DAP_REG      = 0x45,
    STLINK_DEBUG_APIV2_WRITE_DAP_REG     = 0x46,
    STLINK_DEBUG_APIV2_READMEM_16BIT     = 0x47,
    STLINK_DEBUG_APIV2_WRITEMEM_16BIT    = 0x48,
    STLINK_DEBUG_APIV2_INIT_AP           = 0x4B,
    STLINK_DEBUG_APIV2_CLOSE_AP_DBG      = 0x4C,

    STLINK_DEBUG_WRITEMEM_32BIT_NO_ADDR_INC = 0x50,
    STLINK_DEBUG_READMEM_32BIT_NO_ADDR_INC  = 0x54,
    STLINK_APIV3_SET_COM_FREQ            = 0x61,
    STLINK_APIV3_GET_COM_FREQ            = 0x62,
    STLINK_DEBUG_ENTER_SWD               = 0xa3,
    STLINK_APIV3_GET_VERSION_EX          = 0xFB
};

/*  Globals */
libusb_device_handle *gdev_handle;
libusb_context       *gusbcontext;

/*  Functions */
void Stlink_Usb_Init (void);
void Stlink_Open (void);
void Stlink_Swim_Cmd (uint32_t cmd);
void Stlink_Write_Byte (uint32_t address, uint32_t byte);
void Stlink_Write_Word (uint32_t address, uint32_t word);
uint32_t Stlink_Get_Mode (void);
uint32_t Stlink_Get_Swim_Status (void);
uint32_t Stlink_Read_Byte (uint32_t address);
uint32_t Stlink_Read_Word (uint32_t address);
void Stlink_Prog_Byte (uint32_t address, uint32_t byte);
void Stlink_Prog_Dword (uint32_t address, uint32_t dword);
int  Stlink_Prog_Block (uint32_t blk_add, uint32_t blk_size,
    unsigned char *blk_data, unsigned char *blk_def);
void Stlink_Read_Memory (uint32_t address, uint32_t size, FILE *file);
void Stlink_Read_Block (uint32_t address, uint32_t size, unsigned char *data);
