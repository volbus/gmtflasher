

void
Stlink_Usb_Init (void)
{
  libusb_device **devs;
  int i, k;

  /* Initialize libusb. This function must be called before calling any other
   * libusb function.
   */
  k = libusb_init (&gusbcontext);
  if (k) {
    printf ("%s:%s:%i: %s\n", __FILE__, __func__, __LINE__,
        libusb_error_name (k));
    exit (EXIT_FAILURE);
  }

  /* Returns a list of USB devices currently attached to the system. This is
   * your entry point into finding a USB device to operate. The return value of
   * this function indicates the number of devices in the resultant list. The
   * list is actually one element larger, as it is NULL-terminated.
   */
  ssize_t cnt = libusb_get_device_list (gusbcontext, &devs);
  if (cnt<1) {
    printf ("%s:%s:%i: %s\n", __FILE__, __func__, __LINE__,
        libusb_error_name (cnt));
    exit (EXIT_FAILURE);
  }

  for (i=0; i<(cnt-1); i++) {
    struct libusb_device_descriptor desc;

    k = libusb_get_device_descriptor(devs[i], &desc);
    if (k) {
      printf ("%s:%s:%i: %s\n", __FILE__, __func__, __LINE__,
          libusb_error_name (k));
      libusb_free_device_list (devs, 1);
      exit (EXIT_FAILURE);
    }
    if (desc.idVendor==STLINK_USB_VENDOR_ID
        && desc.idProduct==STLINK_USB_PRODUCT_ID)
      break;
  }

  if (i==(cnt-1)) {
    libusb_free_device_list (devs, 1);
    printf ("No STLinkV2 device connected!\n");
    exit (EXIT_FAILURE);
  }

  /* We found the StLink, devs[i], try to open it */
  k = libusb_open (devs[i], &gdev_handle);
  switch (k) {
  case 0:
    break;
  case LIBUSB_ERROR_NO_MEM:
    printf (
        "libusb_open() returned error: memory allocation failure\n");
    break;
  case LIBUSB_ERROR_ACCESS:
    printf (
        "libusb_open() returned error: user has insufficient permissions\n");
    break;
  case LIBUSB_ERROR_NO_DEVICE:
    printf (
        "libusb_open() returned error: device has been disconnected\n");
    break;
  default:
    printf ("libusb_open() returned error: %i\n", k);
  }
  if (k) {
    libusb_free_device_list (devs, 1);
    exit (EXIT_FAILURE);
  }

  /* After we opened the device, we must free the list and unref the devices in
   * the list
   */
  libusb_free_device_list (devs, 1);

  /* Determine if a kernel driver is active on an interface. If a kernel driver
   * is active, you cannot claim the interface, and libusb will be unable to
   * perform I/O.
   */
  k = libusb_kernel_driver_active (gdev_handle, 0);
  switch (k) {
  case 0:
  //no kernel driver active
    break;
  case 1:
  //a kernel driver is active
    if (libusb_detach_kernel_driver(gdev_handle, 0)) {
      printf ("libusb_detach_kernel_driver() returned error\n");
      exit (EXIT_FAILURE);
    }
    break;
  case LIBUSB_ERROR_NO_DEVICE:
    printf (
        "libusb_kernel_driver_active() returned error: no device\n");
    exit (EXIT_FAILURE);
    break;
  case LIBUSB_ERROR_NOT_SUPPORTED:
    printf (
        "libusb_kernel_driver_active() returned error: not supported\n");
    exit (EXIT_FAILURE);
    break;
  default:
  //other error
    printf ("libusb_kernel_driver_active() returned error: %i\n", k);
    exit (EXIT_FAILURE);
  }

  if (libusb_claim_interface (gdev_handle, 0)) {
    printf ("libusb_claim_interface: unable to claim interface\n");
    exit (EXIT_FAILURE);
  }
}

static void
usb_tx_cmd (unsigned char *buf)
{
  int txcnt = 0;
  int try = 0;
  int q;

utc_try:
  q = libusb_bulk_transfer (gdev_handle, STLINK_USB_ENDPOINT_OUT2, buf, 16,
      &txcnt, 100);
  if (q) {
    printf ("%s:%s:%d: %s, buf[0,1]=0x%02X%02X\n", __FILE__, __func__,
        __LINE__, libusb_error_name (q), buf[0], buf[1]);
    if (q==LIBUSB_ERROR_PIPE) {
    //endpoint halted
      try++;
      if (try>=2)
        exit (EXIT_FAILURE);
      libusb_clear_halt (gdev_handle, STLINK_USB_ENDPOINT_OUT2);
      usleep (2000);
      goto utc_try;
    } else {
      //libusb_reset_device (gdev_handle);
      exit (EXIT_FAILURE);
    }
  }
  if(txcnt != 16) {
    printf ("%s:%s:%d: libusb_bulk_transfer: wrong number of tx bytes,"
        " asked 16, transmitted %d\n", __FILE__, __func__, __LINE__, txcnt);
    exit (EXIT_FAILURE);
  }
}

static void
usb_rx (unsigned char *buf, int cnt)
{
  int rxcnt = 0;
  int try = 0;
  int q;

ur_try:
  q = libusb_bulk_transfer (gdev_handle, STLINK_USB_ENDPOINT_IN1, buf, cnt,
      &rxcnt, 100);
  if (q) {
    printf ("%s:%s:%d: %s\n", __FILE__, __func__,__LINE__,
        libusb_error_name (q));
    if (q==LIBUSB_ERROR_PIPE) {
    //endpoint halted
      try++;
      if (try>=2)
        exit (EXIT_FAILURE);
      libusb_clear_halt (gdev_handle, STLINK_USB_ENDPOINT_IN1);
      usleep (2000);
      goto ur_try;
    } else {
      exit (EXIT_FAILURE);
    }
  }
  if(rxcnt != cnt) {
    printf ("%s:%s:%d: libusb_bulk_transfer: wrong number of rx bytes,"
        " asked %d, received %d\n", __FILE__, __func__, __LINE__, cnt, rxcnt);
    exit (EXIT_FAILURE);
  }
}

void
Stlink_Swim_Cmd (uint32_t cmd)
{
  unsigned char buf[16];

  memset (buf, 0x00, sizeof(buf));
  buf[0] = STLINK_SWIM_COMMAND;
  buf[1] = cmd;
  usb_tx_cmd (buf);
}

uint32_t
Stlink_Get_Mode (void)
{
  unsigned char buf[16];

  memset (buf, 0x00, sizeof(buf));
  buf[0] = STLINK_GET_CURRENT_MODE;
  usb_tx_cmd (buf);
  usb_rx (buf, 2);
  return (buf[0]<<8) | buf[1];
}

uint32_t
Stlink_Get_Swim_Status (void)
{
  unsigned char buf[16];

  memset (buf, 0x00, sizeof(buf));
  buf[0] = STLINK_SWIM_COMMAND;
  buf[1] = STLINK_SWIM_READSTATUS;
  usb_tx_cmd (buf);
  usb_rx (buf, 4);
  return (buf[3]<<24) | (buf[2]<<16) | (buf[1]<<8) | buf[0];
}

static uint32_t
stlink_wait_swim_idle (void)
{
  uint32_t q;

  for (int i=8; i>0; i--) {
    usleep (2000);
    q = Stlink_Get_Swim_Status ();
    if (!(q & 0xFF))
      return 0;
  }
  return q;
}

void
Stlink_Write_Byte (uint32_t address, uint32_t byte)
{
  unsigned char buf[16];

  memset (buf, 0x00, sizeof(buf));
  buf[0] = STLINK_SWIM_COMMAND;
  buf[1] = STLINK_SWIM_WRITEMEM;
  //cnt
  buf[2] = 0x00;
  buf[3] = 0x01;
  //address
  buf[4] = 0x00;
  buf[5] = address>>16;
  buf[6] = address>>8;
  buf[7] = address;
  //byte
  buf[8] = byte;
  usb_tx_cmd (buf);

  uint32_t stat = stlink_wait_swim_idle ();
  if (stat) {
    printf ("Error, %s: SWIM status returned 0x%02X\n", __func__,
        stat);
    exit (EXIT_FAILURE);
  }
}

void
Stlink_Write_Word (uint32_t address, uint32_t word)
{
  unsigned char buf[16];

  memset (buf, 0x00, sizeof(buf));
  buf[0] = STLINK_SWIM_COMMAND;
  buf[1] = STLINK_SWIM_WRITEMEM;
  //cnt
  buf[2] = 0x00;
  buf[3] = 0x02;
  //address
  buf[4] = 0x00;
  buf[5] = address>>16;
  buf[6] = address>>8;
  buf[7] = address;
  //word
  buf[8] = word>>8;
  buf[9] = word;
  usb_tx_cmd (buf);

  uint32_t stat = stlink_wait_swim_idle ();
  if (stat) {
    printf ("Error, %s: SWIM status returned 0x%02X\n", __func__,
        stat);
    exit (EXIT_FAILURE);
  }
}

void
Stlink_Open (void)
{
  unsigned char buf[16];
  uint32_t q;

//read stlink version
  PRINT_IF_VERBOSE ("...read version STlink/JTAG/SWIM: ");
  memset (buf, 0x00, sizeof(buf));
  buf[0] = STLINK_GET_VERSION;
  usb_tx_cmd (buf);
  usb_rx (buf, 6);
  PRINT_IF_VERBOSE ("%d/%d/%d\n", buf[0]>>4, ((buf[0]&0x0F)<<2)|(buf[1]>>6),
      buf[1]&0x3F);

//read current mode and set to swim
  q = Stlink_Get_Mode();
  switch (q) {
    case STLINK_MODE_DFU:
      PRINT_IF_VERBOSE ("...stlink mode: DFU (Direct Firmware Update)\n..."
          "exiting DFU mode: ");
      memset (buf, 0x00, sizeof(buf));
      buf[0] = STLINK_DFU_COMMAND;
      buf[1] = STLINK_DFU_EXIT;
      usb_tx_cmd (buf);
      PRINT_IF_VERBOSE ("done\n");
      break;
    case STLINK_MODE_MASS:
      PRINT_IF_VERBOSE ("...stlink mode: MASS (Mass Storage Device)\n..."
          "exiting MASS mode: ");
      memset (buf, 0x00, sizeof(buf));
      buf[0] = STLINK_DFU_COMMAND;
      buf[1] = STLINK_DFU_EXIT;
      usb_tx_cmd (buf);
      PRINT_IF_VERBOSE ("done\n");
      break;
    case STLINK_MODE_DEBUG:
      PRINT_IF_VERBOSE ("...stlink debug\n...exiting Debug mode: ");
      //should we issue (STLINK_MODE_DEBUG + STLINK_DEBUG_EXIT) ???
      memset (buf, 0x00, sizeof(buf));
      buf[0] = STLINK_DFU_COMMAND;
      buf[1] = STLINK_DFU_EXIT;
      usb_tx_cmd (buf);
      PRINT_IF_VERBOSE ("done\n");
      break;
    case STLINK_MODE_SWIM:
      PRINT_IF_VERBOSE ("...stlink mode: SWIM_MODE\n");
      break;
    case STLINK_MODE_BOOTLOADER:
      PRINT_IF_VERBOSE ("...stlink mode: BOOTLOADER_MODE\n");
      //what's to be done???
      printf ("Sorry, we don't know how to handle this mode!\n");
      exit (EXIT_FAILURE);
    default:
      printf ("...unknown stlink_mode: 0x%04X\n", q);
      exit (EXIT_FAILURE);
  }

  if (q != STLINK_MODE_SWIM) {
    PRINT_IF_VERBOSE ("...entering SWIM mode: ");
    memset (buf, 0x00, sizeof(buf));
    buf[0] = STLINK_SWIM_COMMAND;
    buf[1] = STLINK_SWIM_ENTER;
    usb_tx_cmd (buf);
    q = Stlink_Get_Mode ();
    if (q == STLINK_MODE_SWIM) {
      PRINT_IF_VERBOSE ("done\n");
    } else {
      PRINT_IF_VERBOSE ("error, %X\n", q);
      exit (EXIT_FAILURE);
    }
  }

//read target Vcc
  PRINT_IF_VERBOSE ("...reading target Vcc: ");
  memset (buf, 0x00, sizeof(buf));
  buf[0] = STLINK_GET_TARGET_VOLTAGE;
  usb_tx_cmd (buf);
  usb_rx (buf, 8);
  uint32_t factor  = (buf[3]<<24) | (buf[2]<<16) | (buf[1]<<8) | (buf[0]);
  uint32_t reading = (buf[7]<<24) | (buf[6]<<16) | (buf[5]<<8) | (buf[4]);

  q = 2400*reading/factor;
  if (q < 1500) {
    if (prog_mode & PROG_MODE_VERBOSE)
      printf ("%d mV, no target connected?\n", q);
    else
      printf ("...target Vcc: %d mV, no target connected?\n", q);
  } else {
    if (prog_mode & PROG_MODE_VERBOSE)
      printf ("%d mV\n", q);
  }

//now we activate the swim connection to device
  PRINT_IF_VERBOSE ("...activate SWIM connection to µC: ");
  //NRES \_
  Stlink_Swim_Cmd (STLINK_SWIM_NRES_LOW);
  if (stlink_wait_swim_idle ()) {
    if (prog_mode & PROG_MODE_VERBOSE)
      printf (" NRES pull low error!\n");
    else
      printf ("...NRES pull low error!\n");
    exit (EXIT_FAILURE);
  }

  Stlink_Swim_Cmd (STLINK_SWIM_ENTER_SEQ);
  if (stlink_wait_swim_idle ()) {
    if (prog_mode & PROG_MODE_VERBOSE)
      printf (" SWIM activation error!\n");
    else
      printf ("...SWIM activation error!\n");
    exit (EXIT_FAILURE);
  }

  Stlink_Write_Byte (0x7F80, 0xA5);

  //we can now release NRES
  Stlink_Swim_Cmd (STLINK_SWIM_NRES_HIGH);
  if (stlink_wait_swim_idle ()) {
    if (prog_mode & PROG_MODE_VERBOSE)
      printf (" NRES release error!\n");
    else
      printf ("...NRES release error!\n");
    exit (EXIT_FAILURE);
  }

  //reset swim for better clk sync
  Stlink_Swim_Cmd (STLINK_SWIM_RESET);
  if (stlink_wait_swim_idle ()) {
    if (prog_mode & PROG_MODE_VERBOSE)
      printf (" SWIM reset error!\n");
    else
      printf ("...SWIM reset error!\n");
    exit (EXIT_FAILURE);
  }

  Stlink_Write_Byte (STM8_DM_CSR2, 0x08);

  if (prog_mode & PROG_MODE_VERBOSE)
    printf ("done\n");
}

uint32_t
Stlink_Read_Byte (uint32_t address)
{
  unsigned char buf[16];

  memset (buf, 0x00, sizeof(buf));
  buf[0] = STLINK_SWIM_COMMAND;
  buf[1] = STLINK_SWIM_READMEM;
  //uint16 cnt
  buf[2] = 0x00;
  buf[3] = 0x01;
  //uint32 address
  buf[4] = 0x00;
  buf[5] = address>>16;
  buf[6] = address>>8;
  buf[7] = address;
  usb_tx_cmd (buf);

  uint32_t stat = stlink_wait_swim_idle ();
  if (stat) {
    printf ("Error, %s: SWIM status returned 0x%X\n", __func__, stat);
    exit (EXIT_FAILURE);
  }

  Stlink_Swim_Cmd (STLINK_SWIM_READBUF);
  usb_rx (buf, 1);

  return buf[0];
}


uint32_t
Stlink_Read_Word (uint32_t address)
{
  unsigned char buf[16];

  memset (buf, 0x00, sizeof(buf));
  buf[0] = STLINK_SWIM_COMMAND;
  buf[1] = STLINK_SWIM_READMEM;
  //uint16 cnt
  buf[2] = 0x00;
  buf[3] = 0x02;
  //uint32 address
  buf[4] = 0x00;
  buf[5] = address>>16;
  buf[6] = address>>8;
  buf[7] = address;
  usb_tx_cmd (buf);

  uint32_t stat = stlink_wait_swim_idle ();
  if (stat) {
    printf ("Error, %s: SWIM status returned 0x%X\n", __func__, stat);
    exit (EXIT_FAILURE);
  }

  Stlink_Swim_Cmd (STLINK_SWIM_READBUF);
  usb_rx (buf, 2);

  return ((buf[0]<<8) | buf[1]);
}


uint32_t
stlink_read_dword (uint32_t address)
{
  unsigned char buf[16];

  memset (buf, 0x00, sizeof(buf));
  buf[0] = STLINK_SWIM_COMMAND;
  buf[1] = STLINK_SWIM_READMEM;
  //uint16 cnt
  buf[2] = 0x00;
  buf[3] = 0x04;
  //uint32 address
  buf[4] = 0x00;
  buf[5] = address>>16;
  buf[6] = address>>8;
  buf[7] = address & 0xFC;
  usb_tx_cmd (buf);

  uint32_t stat = stlink_wait_swim_idle ();
  if (stat) {
    printf ("Error, %s: SWIM status returned 0x%X\n", __func__, stat);
    exit (EXIT_FAILURE);
  }

  Stlink_Swim_Cmd (STLINK_SWIM_READBUF);
  usb_rx (buf, 4);

  return ((buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3]);
}


void
Stlink_Unlock_Memory (mcu *uc, uint32_t address)
{
  if ( (address >= uc->eeprom_add && address < (uc->eeprom_add + uc->eeprom_size))
      || (address >= 0x4800 && address < 0x4880) ) {
  //eeprom or option bytes
    if (prog_stat & PROG_STAT_UL_EEPROM)
      return;
    //write FLASH_DUKR register with the key unlock
    if (prog_mode & PROG_MODE_STM8L) {
    //stm8l type
      Stlink_Write_Byte (0x5053, 0xAE);
      Stlink_Write_Byte (0x5053, 0x56);
      if ( !(Stlink_Read_Byte (0x5054) & 0x08) ) {
        printf ("Could not unlock EEPROM memory!\n");
        exit (EXIT_FAILURE);
      }
    } else {
    //stm8s type
      Stlink_Write_Byte (0x5064, 0xAE);
      Stlink_Write_Byte (0x5064, 0x56);
      if ( !(Stlink_Read_Byte (0x505F) & 0x08) ) {
        printf ("Could not unlock EEPROM memory!\n");
        exit (EXIT_FAILURE);
      }
    }
    prog_stat |= PROG_STAT_UL_EEPROM;
  } else if (address >= 0x8000) {
  //flash
    if (prog_stat & PROG_STAT_UL_FLASH)
      return;
    //write FLASH_PUKR register with the key unlock
    if (prog_mode & PROG_MODE_STM8L) {
    //stm8l type
      Stlink_Write_Byte (0x5052, 0x56);
      Stlink_Write_Byte (0x5052, 0xAE);
      if ( !(Stlink_Read_Byte (0x5054) & 0x02) ) {
        printf ("Could not unlock FLASH memory!\n");
        exit (EXIT_FAILURE);
      }
    } else {
    //stm8s type
      Stlink_Write_Byte (0x5062, 0x56);
      Stlink_Write_Byte (0x5062, 0xAE);
      if ( !(Stlink_Read_Byte (0x505F) & 0x02) ) {
        printf ("Could not unlock FLASH memory!\n");
        exit (EXIT_FAILURE);
      }
    }
    prog_stat |= PROG_STAT_UL_FLASH;
    return;
  } else {
    return;
  }
}

static void
programm_block (uint32_t blk_add, uint32_t blk_size, unsigned char *blk_data)
{
  unsigned char buf[16];
  uint32_t iaspr;

  buf[0] = STLINK_SWIM_COMMAND;
  buf[1] = STLINK_SWIM_WRITEMEM;
  //cnt
  buf[2] = 0x00;
  buf[3] = blk_size;
  //address
  buf[4] = 0x00;
  buf[5] = blk_add>>16;
  buf[6] = blk_add>>8;
  buf[7] = blk_add & ~(blk_size-1);
  memcpy (buf+8, blk_data, 8);

  //block programming enable
  if (prog_mode & PROG_MODE_STM8L) {
  //stm8l type
    Stlink_Write_Byte (0x5051, 0x01);
    iaspr = 0x5054;
  } else {
  //stm8s type
    Stlink_Write_Byte (0x505B, 0x01);
    Stlink_Write_Byte (0x505C, 0xFE);
    iaspr = 0x505F;
  }
  usb_tx_cmd (buf);
  //send the rest of the data block
  int txcnt;
  int q;

  q = libusb_bulk_transfer (gdev_handle, STLINK_USB_ENDPOINT_OUT2, blk_data + 8,
      blk_size - 8, &txcnt, 100);
  if (q) {
    printf ("%s:%d: %s\n", __func__,__LINE__, libusb_error_name (q));
    exit (EXIT_FAILURE);
  }
  if(txcnt != (blk_size - 8)) {
    printf ("libusb_bulk_transfer: wrong number of tx bytes, "
        "asked %d, transmitted %d\n", blk_size - 8, txcnt);
    exit (EXIT_FAILURE);
  }

  usleep(6000);

  for (int i=8; i>0; i--) {
    if ( Stlink_Read_Byte (iaspr) & 0x04 )
      return;
    usleep (2000);
  }

  printf ("block programming error, address=0x%04X\n", blk_add);
  exit (EXIT_FAILURE);
}


void
Stlink_Prog_Byte (uint32_t address, uint32_t byte)
{
  unsigned char buf[16];
  uint32_t iaspr;

  if (address>=0x4800 && address<0x4840) {
  //OPT
    if (prog_mode & PROG_MODE_STM8L) {
    //stm8l type
      Stlink_Write_Byte (0x5051, 0x80);
    } else {
    //stm8s type
      Stlink_Write_Byte (0x505B, 0x80);
      Stlink_Write_Byte (0x505C, 0x7F);
    }

  }

  memset (buf, 0x00, sizeof(buf));
  buf[0] = STLINK_SWIM_COMMAND;
  buf[1] = STLINK_SWIM_WRITEMEM;
  //cnt
  buf[2] = 0x00;
  buf[3] = 0x01;
  //address
  buf[4] = 0x00;
  buf[5] = address>>16;
  buf[6] = address>>8;
  buf[7] = address;
  //word
  buf[8] = byte;
  usb_tx_cmd (buf);

  (prog_mode & PROG_MODE_STM8L) ? (iaspr = 0x5054) : (iaspr = 0x505F);
  usleep(6000);

  for (int i=8; i>0; i--) {
    if ( Stlink_Read_Byte(iaspr) & 0x04 )
      return;
    usleep (2000);
  }

  printf ("byte programming error, address=0x%04X, byte=0x%02X\n",
      address, byte);
  exit (EXIT_FAILURE);
}


void
Stlink_Prog_Dword (uint32_t address, uint32_t dword)
{
  unsigned char buf[16];
  uint32_t iaspr;

  //word programming enable
  if (prog_mode & PROG_MODE_STM8L) {
  //stm8l type
    Stlink_Write_Byte (0x5051, 0x40);
    iaspr = 0x5054;
  } else {
  //stm8s type
    Stlink_Write_Byte (0x505B, 0x40);
    Stlink_Write_Byte (0x505C, 0xBF);
    iaspr = 0x505F;
  }

  memset (buf, 0x00, sizeof(buf));
  buf[0] = STLINK_SWIM_COMMAND;
  buf[1] = STLINK_SWIM_WRITEMEM;
  //cnt
  buf[2] = 0x00;
  buf[3] = 0x04;
  //address
  buf[4] = 0x00;
  buf[5] = address>>16;
  buf[6] = address>>8;
  buf[7] = address & 0xFC;
  //word
  buf[8] = dword>>24;
  buf[9] = dword>>16;
  buf[10] = dword>>8;
  buf[11] = dword;
  usb_tx_cmd (buf);

  usleep(6000);

  for (int i=8; i>0; i--) {
    if ( Stlink_Read_Byte (iaspr) & 0x04 )
      return;
    usleep (2000);
  }

  printf ("dword programming error, address=0x%04X, dword=0x%08X\n",
      address, dword);
  exit (EXIT_FAILURE);
}


/* The function programms selectively the data block starting at address
 * blk_add, with the data from *blk_data defined by *blk_def, and returns -1 if
 * nothing was written (due to identical data in µC), 0 if the full block was
 * written or the number of 4-byte words written (in case 1 or max. 2 4-byte
 * words differ).
 */
int
Stlink_Prog_Block (uint32_t blk_add, uint32_t blk_size, unsigned char *blk_data,
    unsigned char *blk_def)
{
  // If force flag is set we write all block data
  if (prog_mode & PROG_MODE_FORCE_ALL) {
    programm_block (blk_add, blk_size, blk_data);
    return 0;
  }

  unsigned char *ucblock = malloc (blk_size);
  MALLOC_TST (ucblock);
  Stlink_Read_Block (blk_add, blk_size, ucblock);

  // If µC block has the same content as the block to write, skip the write
  if (!memcmp (ucblock, blk_data, blk_size)) {
    free (ucblock);
    return -1;
  }

  // Count the number of 4-byte words different in the 2 blocks
  uint32_t k = 0;

  for (int i=0; i<blk_size; i+=4) {
    uint32_t mask = *((uint32_t *)(blk_def + i));
    uint32_t d0 = *((uint32_t *)(blk_data + i)) & mask;
    uint32_t d1 = *((uint32_t *)(ucblock + i)) & mask;
    if (d0 != d1)
      k++;
  }
  if (k==0)
	return -1;

  // If more than 2 4-byte words are different, we write the full block
  if (k > 2) {
    /* before we write the block data, we fill in the persistent bytes if flag
     * set
     */
    if (prog_mode & PROG_MODE_PERSIST) {
      for (int i=0; i<blk_size; i++) {
        if ( *(blk_def + i) )
          *(ucblock + i) = *(blk_data + i);
      }
      programm_block (blk_add, blk_size, ucblock);
    } else {
      programm_block (blk_add, blk_size, blk_data);
    }
    free (ucblock);
    return 0;
  }

  /* Here we only have 1 or 2 4-byte words different in the 2 blocks, we need to
   * scan and selectively programm them
   */
  int cnt = 0;

  for (int i=0; i<blk_size; i+=4) {
    uint32_t mask = *((uint32_t *)(blk_def + i));
    uint32_t d0 = *((uint32_t *)(blk_data + i)) & mask;
    uint32_t d1 = *((uint32_t *)(ucblock + i)) & mask;
    if (d0 != d1) {
      *(blk_def+i) ? (k=*(blk_data+i)) : (k=*(ucblock+i));
      k<<=8;
      *(blk_def+i+1) ? (k|=*(blk_data+i+1)) : (k|=*(ucblock+i+1));
      k<<=8;
      *(blk_def+i+2) ? (k|=*(blk_data+i+2)) : (k|=*(ucblock+i+2));
      k<<=8;
      *(blk_def+i+3) ? (k|=*(blk_data+i+3)) : (k|=*(ucblock+i+3));
      Stlink_Prog_Dword (blk_add + i, k);
      cnt++;
    }
  }

  free (ucblock);
  return cnt;
}


void
Stlink_Read_Memory (uint32_t address, uint32_t size, FILE *file)
{
  uint32_t cnt;
  unsigned char buf[64];
  uint32_t off = 0;

  while (size) {
    (size < 64) ? (cnt = size) : (cnt = 64);

    memset (buf, 0x00, 16);
    buf[0] = STLINK_SWIM_COMMAND;
    buf[1] = STLINK_SWIM_READMEM;
    //uint16 cnt
    buf[2] = 0x00;
    buf[3] = cnt;
    //uint32 address
    buf[4] = 0x00;
    buf[5] = address>>16;
    buf[6] = address>>8;
    buf[7] = address;
    usb_tx_cmd (buf);

    uint32_t stat = stlink_wait_swim_idle ();
    if (stat) {
      printf ("Error, %s: SWIM status returned 0x%02X\n", __func__,
          stat);
      exit (EXIT_FAILURE);
    }

    Stlink_Swim_Cmd (STLINK_SWIM_READBUF);
    usb_rx (buf, cnt);

    if ( (address - off + cnt) > 0x10000 ) {
      off = address & 0xFFFF0;
      fprintf (file, ":02000002%04X%02X\n", off>>4, 0xFF & (0x100 -
          (0xFF & (0x04 + (off>>12) + ((off>>4)&0xFF) )) ) );
    }

    Ihex_Wr_Data (buf, address - off, cnt, file);
    address += cnt;
    size -= cnt;
  }
}

void
Stlink_Read_Block (uint32_t address, uint32_t size, unsigned char *data)
{
  uint32_t cnt, i;
  unsigned char buf[16];

  i = 0;
  while (size) {
    (size < 64) ? (cnt = size) : (cnt = 64);

    memset (buf, 0x00, sizeof(buf));
    buf[0] = STLINK_SWIM_COMMAND;
    buf[1] = STLINK_SWIM_READMEM;
    //uint16 cnt
    buf[2] = 0x00;
    buf[3] = cnt;
    //uint32 address
    buf[4] = 0x00;
    buf[5] = address>>16;
    buf[6] = address>>8;
    buf[7] = address;
    usb_tx_cmd (buf);

    uint32_t stat = stlink_wait_swim_idle ();
    if (stat) {
      printf ("Error, %s: SWIM status returned 0x%02X\n", __func__,
          stat);
      exit (EXIT_FAILURE);
    }

    Stlink_Swim_Cmd (STLINK_SWIM_READBUF);
    usb_rx (data + i, cnt);

    address += cnt;
    size -= cnt;
    i += cnt;
  }
}
