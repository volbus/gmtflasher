/* Main source file for GmtFlasher
 * Cristian Gyorgy, 2021 */

#include "gmtflasher.h"
#include "help.h"


static void
show_version (void)
{
  printf ("GmtFlasher %s, STM8 programming tool for STLinkV2\n",
      SOFTWARE_VERSION);
}


static void
show_help (void)
{
  show_version ();
  puts (help_text);
}


void
exit_handler (void)
{
  if (ghexfile)
    fclose (ghexfile);
  if (gdev_handle) {
    libusb_release_interface (gdev_handle, 0);
    libusb_close (gdev_handle);
  }
  if (gusbcontext)
    libusb_exit (gusbcontext);
}

/* Reads mcu memory according to job and writes the data into a intel hex file.
 * The name of the file is given by the -o option if defined, else is a fix
 * path/name, depending on job.
 */
static void
read_mcu (int job, mcu *uc)
{
  char rfname[64];
  FILE *rfile;

  //setup file name
  if (prog_stat & PROG_STAT_OFILE) {
    strncpy (rfname, ofile_name, sizeof(rfname));
    rfname[sizeof(rfname)-1] = 0x00;
  } else {
    switch (job) {
    case JOB_READ_ALL:
      strcpy (rfname, "/tmp/gmtflasher/mcu_rd.ihx");
      break;
    case JOB_READ_FLASH:
      strcpy (rfname, "/tmp/gmtflasher/flash_rd.ihx");
      break;
    case JOB_READ_EEPROM:
      strcpy (rfname, "/tmp/gmtflasher/eeprom_rd.ihx");
      break;
    case JOB_READ_OPT:
      strcpy (rfname, "/tmp/gmtflasher/opt_rd.ihx");
      break;
    case JOB_READ_RANGE:
      strcpy (rfname, "/tmp/gmtflasher/range_rd.ihx");
      break;
    }
  }

  //open read file
  rfile = fopen (rfname, "w+");
  if (!rfile) {
    printf (strerror(errno));
    printf ("\n");
    exit (EXIT_FAILURE);
  }

  //do actual reading from mcu to file
  switch (job) {
  case JOB_READ_ALL:
    printf ("...reading device: ");
    fflush (stdout);
    Stlink_Read_Memory (uc->eeprom_add, uc->eeprom_size, rfile);
    Stlink_Read_Memory (0x4800, uc->block_size, rfile);
    Stlink_Read_Memory (0x8000, uc->flash_size, rfile);
    break;
  case JOB_READ_FLASH:
    printf ("...reading FLASH: ");
    fflush (stdout);
    Stlink_Read_Memory (0x8000, uc->flash_size, rfile);
    break;
  case JOB_READ_EEPROM:
    printf ("...reading EEPROM: ");
    fflush (stdout);
    Stlink_Read_Memory (uc->eeprom_add, uc->eeprom_size, rfile);
    break;
  case JOB_READ_OPT:
    printf ("...reading OPT: ");
    fflush (stdout);
    Stlink_Read_Memory (0x4800, uc->block_size, rfile);
    break;
  case JOB_READ_RANGE:
    printf ("...reading address range [0x%X, 0x%X): ", uc->add_0, uc->add_1);
    fflush (stdout);
    Stlink_Read_Memory (uc->add_0, uc->add_1 - uc->add_0, rfile);
    break;
  }
  fprintf (rfile, ":00000001FF");
  fclose (rfile);
  printf ("done\n");
  printf ("See file %s\n", rfname);
}


int
main (int argc, char **argv)
{
  int           job = 0;
  mcu           uc;
  int           mblocks;
  uint32_t      *blk_add = NULL;
  unsigned char *data = NULL;
  unsigned char *ddef = NULL;

  if ( atexit (exit_handler) ) {
    printf (strerror(errno));
    exit(EXIT_FAILURE);
  }

  memset (&uc, 0x00, sizeof(uc));

//check user arguments, identify jobs and options
  for (int i=1; i<argc; i++) {
    if ( !strcasecmp(argv[i], "-u") ) {
      i++;
      if (i>=argc) {
        printf ("Missing argument for -u option!\n");
        exit (EXIT_FAILURE);
      }
      strncpy (uc.name, argv[i], sizeof (uc.name) - 1);
      uc.name[sizeof(uc.name)] = 0x00;
    } else if ( !strcasecmp(argv[i], "-o") ) {
      i++;
      if (i>=argc) {
        printf ("Missing argument for -u option!\n");
        exit (EXIT_FAILURE);
      }
      ofile_name = argv[i];
      prog_stat |= PROG_STAT_OFILE;
    } else if ( !strcasecmp(argv[i], "-w") ) {
      job |= JOB_WRITE_ALL;
    } else if ( !strcasecmp(argv[i], "-r") ) {
      job |= JOB_READ_ALL;
    } else if ( !strcasecmp(argv[i], "-wf") ) {
      job |= JOB_WRITE_FLASH;
    } else if ( !strcasecmp(argv[i], "-we") ) {
      job |= JOB_WRITE_EEPROM;
    } else if ( !strcasecmp(argv[i], "-wo") ) {
      job |= JOB_WRITE_OPT;
    } else if ( !strcasecmp(argv[i], "-ul") ) {
      job |= JOB_UNLOCK;
    } else if ( !strcasecmp(argv[i], "-lo") ) {
      job |= JOB_LOCK;
    } else if ( !strcasecmp(argv[i], "-rf") ) {
      job |= JOB_READ_FLASH;
    } else if ( !strcasecmp(argv[i], "-re") ) {
      job |= JOB_READ_EEPROM;
    } else if ( !strcasecmp(argv[i], "-ro") ) {
      job |= JOB_READ_OPT;
    } else if ( !strcasecmp(argv[i], "-rr") ) {
      job |= JOB_READ_RANGE;
      i++;
      if (i>=argc) {
        printf ("Missing argument for -rr option!\n");
        exit (EXIT_FAILURE);
      }
    } else if ( !strcasecmp(argv[i], "-wb") ) {
      job |= JOB_WRITE_BYTE;
      i += 2;
      if (i>=argc) {
        printf ("Missing argument for -wb option!\n");
        exit (EXIT_FAILURE);
      }
    } else if ( !strcasecmp(argv[i], "-rb") ) {
      job |= JOB_READ_BYTE;
      i++;
      if (i>=argc) {
        printf ("Missing argument for -rb option!\n");
        exit (EXIT_FAILURE);
      }
    } else if ( !strcasecmp(argv[i], "-ww") ) {
      job |= JOB_WRITE_WORD;
      i += 2;
      if (i>=argc) {
        printf ("Missing argument for -ww option!\n");
        exit (EXIT_FAILURE);
      }
    } else if ( !strcasecmp(argv[i], "-rw") ) {
      job |= JOB_READ_WORD;
      i++;
      if (i>=argc) {
        printf ("Missing argument for -rw option!\n");
        exit (EXIT_FAILURE);
      }
    } else if ( !strcasecmp(argv[i], "-iw") ) {
      job |= JOB_INC_WORD;
      i++;
      if (i>=argc) {
        printf ("Missing argument for -iw option!\n");
        exit (EXIT_FAILURE);
      }
    } else if ( !strcasecmp(argv[i], "-ib") ) {
      job |= JOB_INC_BYTE;
      i++;
      if (i>=argc) {
        printf ("Missing argument for -ib option!\n");
        exit (EXIT_FAILURE);
      }
    } else if ( !strcasecmp(argv[i], "-v")
        || !strcasecmp(argv[i], "--verbose") ) {
      prog_mode |= PROG_MODE_VERBOSE;
    } else if ( !strcasecmp(argv[i], "-f") ) {
      prog_mode |= PROG_MODE_FORCE_ALL;
    } else if ( !strcasecmp(argv[i], "-p") ) {
      prog_mode |= PROG_MODE_PERSIST;
    } else if ( !strcasecmp(argv[i], "-h") || !strcasecmp(argv[i], "--help") ) {
      job |= JOB_PRINT;
      show_help ();
    } else if ( !strcasecmp(argv[i], "--version") ) {
      job |= JOB_PRINT;
      show_version ();
    } else if ( !strcasecmp(argv[i], "--listmcu") ) {
      job |= JOB_PRINT;
      List_Devices ();
    } else if (i==argc-1) {
      ghexfile_name = argv[i];
    } else {
    //unknown option/argument
      printf ("...Unknown argument \"%s\"\n", argv[i]);
      exit (EXIT_FAILURE);
    }
  }

//exit if no job
  if (job == JOB_PRINT)
    exit (EXIT_SUCCESS);
  if (!job) {
    printf ("No job specified!\n");
    exit (EXIT_SUCCESS);
  }

//exit if no mcu specified
  if (!uc.name[0]) {
    printf ("No µC part number specified!\n");
    exit (EXIT_FAILURE);
  }

//exit if no input hex file and a job that requires an input data file
  if ( (job & (JOB_WRITE_ALL | JOB_WRITE_FLASH | JOB_WRITE_EEPROM | JOB_WRITE_OPT))
      && !ghexfile_name) {
    printf ("Input data file not specified!\n");
    exit (EXIT_FAILURE);
  }

//identify the mcu from the xml file
  PRINT_IF_VERBOSE ("...reading device xml file and identify device: ");
  Get_Xml_Mcu_Data (&uc);
  PRINT_IF_VERBOSE ("done\n");

//if we have an input file, we try to open it
  if (ghexfile_name) {
    PRINT_IF_VERBOSE ("...opening data file %s: ", ghexfile_name);
    ghexfile = fopen (ghexfile_name, "r");
    if (!ghexfile) {
      printf (strerror(errno));
      printf ("\n");
      exit (EXIT_FAILURE);
    }

    mblocks = Ihex_Count_Blocks (ghexfile, uc.block_size);

    blk_add = malloc (mblocks*4);
    MALLOC_TST (blk_add);
    data = malloc (uc.block_size*mblocks);
    MALLOC_TST (data);
    memset (data, 0x00, uc.block_size*mblocks);
    ddef = malloc (uc.block_size*mblocks);
    MALLOC_TST (ddef);
    memset (ddef, 0x00, uc.block_size*mblocks);

    //read the mblocks of data
    Ihex_Read_Data_Blocks (ghexfile, uc.block_size, blk_add, data, ddef);
    PRINT_IF_VERBOSE ("%d blocks of data\n", mblocks);

  /* Aici avem in mblocks numarul de blocuri de date definite in fisier, in *blk_add avem
   * adresele de start la care trebuiesc scrise aceste blolcuri, iar datele se gasesc in
   * data.
   */
  }

//check if /tmp/gmtflasher dir exists, if not create
  if (mkdir ("/tmp/gmtflasher", 0777) && errno!=EEXIST) {
    printf (strerror(errno));
    printf ("\n");
    exit (EXIT_FAILURE);
  }

//usb connection to STLINK
  Stlink_Usb_Init();

//activate SWIM
  Stlink_Open ();

//rescan and execute jobs
  for (int i=1; i<argc; i++) {
    if ( !strcasecmp(argv[i], "-r") ) {
      read_mcu (JOB_READ_ALL, &uc);
    } else if ( !strcasecmp(argv[i], "-rf") ) {
      read_mcu (JOB_READ_FLASH, &uc);
    } else if ( !strcasecmp(argv[i], "-re") ) {
      read_mcu (JOB_READ_EEPROM, &uc);
    } else if ( !strcasecmp(argv[i], "-ro") ) {
      read_mcu (JOB_READ_OPT, &uc);
    } else if ( !strcasecmp(argv[i], "-rr") ) {
      int add0, add1;

      i++;
      if ( (sscanf(argv[i], "%i:%i", &add0, &add1) != 2)
          || (add0 > 0xFFFFFF)
	  || (add1 > 0xFFFFFF) ) {
        printf ("Wrong -rr arguments! Aborted\n");
        exit (EXIT_FAILURE);
      }
      uc.add_0 = add0;
      uc.add_1 = add1;
      read_mcu (JOB_READ_RANGE, &uc);
    } else if ( !strcasecmp(argv[i], "-w") ) {
      int blk_cnt = 0;
      int wrd_cnt = 0;
      int byt_cnt = 0;
      int skip = 0;

      PRINT_IF_VERBOSE ("...writing device: ");
      for (int i=0; i<mblocks; i++) {
        uint32_t add = *(blk_add+i);
        if ( ( add>=0x8000 && add<(0x8000 + uc.flash_size) )
            || ( add>=uc.eeprom_add && add<(uc.eeprom_add + uc.eeprom_size) )
            ) {
          Stlink_Unlock_Memory (&uc, add);
          int q = Stlink_Prog_Block (add, uc.block_size, data+i*uc.block_size,
                ddef+i*uc.block_size);
          if (q==0)
            blk_cnt++;
          else if (q>0)
            wrd_cnt+=q;
          else
            skip++;
        } else if (add>=0x4800 && add<0x4880) {
          Stlink_Unlock_Memory (&uc, add);
          for (int j=0; j<uc.block_size; j++) {
            if (*(ddef+i*uc.block_size+j)) {
              Stlink_Prog_Byte (add+j, *(data+i*uc.block_size+j));
              byt_cnt++;
            }
          }
        }
      }
      printf ("Written %d blocks, %d dwords, %d bytes, skipped %d\n",
            blk_cnt, wrd_cnt, byt_cnt, skip);
    } else if ( !strcasecmp(argv[i], "-wf") ) {
      int blk_cnt = 0;
      int wrd_cnt = 0;
      int skip = 0;

      PRINT_IF_VERBOSE ("...writing FLASH: ");
      for (int i=0; i<mblocks; i++) {
        uint32_t add = *(blk_add+i);
        if ( (add>=0x8000) && (add<(0x8000 + uc.flash_size)) ) {
          Stlink_Unlock_Memory (&uc, add);
          int q = Stlink_Prog_Block (add, uc.block_size, data+i*uc.block_size,
                ddef+i*uc.block_size);
          if (q==0)
            blk_cnt++;
          else if (q>0)
            wrd_cnt+=q;
          else
            skip++;
        }
      }
      if (!(blk_cnt + wrd_cnt + skip))
        printf ("No FLASH data defined in %s\n", ghexfile_name);
      else if (wrd_cnt)
        printf ("Written %d blocks + %d dwords, skipped %d blocks\n",
            blk_cnt, wrd_cnt, skip);
      else
        printf ("Written %d blocks, skipped %d blocks\n", blk_cnt, skip);
    } else if ( !strcasecmp(argv[i], "-we") ) {
      int blk_cnt = 0;
      int wrd_cnt = 0;
      int skip = 0;

      PRINT_IF_VERBOSE ("...writing EEPROM: ");
      for (int i=0; i<mblocks; i++) {
        uint32_t add = *(blk_add+i);
        if ((add>=uc.eeprom_add) && (add<uc.eeprom_add + uc.eeprom_size)) {
          Stlink_Unlock_Memory (&uc, add);
          int q = Stlink_Prog_Block (add, uc.block_size, data+i*uc.block_size,
                ddef+i*uc.block_size);
          if (q==0)
            blk_cnt++;
          else if (q>0)
            wrd_cnt+=q;
          else
            skip++;
        }
      }
      if (!(blk_cnt + wrd_cnt + skip))
        printf ("No EEPROM data defined in %s\n", ghexfile_name);
      else if (wrd_cnt)
        printf ("Written %d blocks + %d dwords, skipped %d blocks\n",
            blk_cnt, wrd_cnt, skip);
      else
        printf ("Written %d blocks, skipped %d blocks\n", blk_cnt, skip);
    } else if ( !strcasecmp(argv[i], "-wo") ) {
      int wr_cnt = 0;

      PRINT_IF_VERBOSE ("...writing OPT: ");
      for (int i=0; i<mblocks; i++) {
        uint32_t add = *(blk_add+i);
        if (add>=0x4800 && add<0x4880) {
          Stlink_Unlock_Memory (&uc, add);
          for (int j=0; j<uc.block_size; j++) {
            if (*(ddef+i*uc.block_size+j)) {
              Stlink_Prog_Byte (add+j, *(data+i*uc.block_size+j));
              wr_cnt++;
            }
          }
        }
      }
      if (!wr_cnt)
        printf ("No OPT data defined in %s\n", ghexfile_name);
      else
        printf ("Written %d OPT data bytes\n", wr_cnt);
    } else if ( !strcasecmp(argv[i], "-ul") ) {
      PRINT_IF_VERBOSE ("...Unlocking device (disable read out protection): ");
      Stlink_Unlock_Memory (&uc, 0x4800);
      if (prog_mode & PROG_MODE_STM8L)
        Stlink_Prog_Byte (0x4800, 0xAA);
      else
        Stlink_Prog_Byte (0x4800, 0x00);
      printf ("done\n");
    } else if ( !strcasecmp(argv[i], "-lo") ) {
      PRINT_IF_VERBOSE ("...Locking device (enable read out protection): ");
      Stlink_Unlock_Memory (&uc, 0x4800);
      if (prog_mode & PROG_MODE_STM8L)
        Stlink_Prog_Byte (0x4800, 0x00);
      else
        Stlink_Prog_Byte (0x4800, 0xAA);
      printf ("done\n");
    } else if ( !strcasecmp(argv[i], "-wb") ) {
      int add;
      int byte;

      i++;
      if ( (sscanf(argv[i], "%i", &add) != 1) || (add > 0xFFFFFF) ) {
        printf ("Wrong -wb argument address parameter! Aborted\n");
        exit (EXIT_FAILURE);
      }
      i++;
      if ( (sscanf(argv[i], "%i", &byte) != 1) || (byte > 0xFF) ) {
        printf ("Wrong -wb argument data parameter! Aborted\n");
        exit (EXIT_FAILURE);
      }
      
      PRINT_IF_VERBOSE ("...writing 0x%02X to address 0x%04X: ", byte, add);
      Stlink_Unlock_Memory (&uc, add);
      Stlink_Prog_Byte (add, byte);
      PRINT_IF_VERBOSE ("done\n");

      PRINT_IF_VERBOSE ("...verify address 0x%04X: ", add);
      if (byte == Stlink_Read_Byte (add)) {
        printf ("done\n");
      } else {
        printf ("failed!\n");
        exit (EXIT_FAILURE);
      }
    } else if ( !strcasecmp(argv[i], "-ww") ) {
      int add;
      int word;
      uint32_t data;

      i++;
      if ( (sscanf(argv[i], "%i", &add) != 1) || (add > 0xFFFFFF) ) {
        printf ("Wrong -ww argument address parameter! Aborted\n");
        exit (EXIT_FAILURE);
      }
      i++;
      if ( (sscanf(argv[i], "%i", &word) != 1) || (word > 0xFFFF) ) {
        printf ("Wrong -ww argument data parameter! Aborted\n");
        exit (EXIT_FAILURE);
      }
      
      PRINT_IF_VERBOSE ("...writing 0x%04X to address 0x%04X: ", add, word);
      Stlink_Unlock_Memory (&uc, add);
      switch (add & 0x03) {
      case 0x00:
        data = stlink_read_dword (add);
        data &= 0xFFFF;
        data |= (word<<16);
        Stlink_Prog_Dword (add, data);
        break;
      case 0x01:
      case 0x03:
        Stlink_Prog_Byte (add+1, word & 0xFF);
        Stlink_Prog_Byte (add, word>>8);
        break;
      case 0x02:
        data = stlink_read_dword (add);
        data &= 0xFFFF0000;
        data |= (word & 0xFFFF);
        Stlink_Prog_Dword (add, data);
      }
      PRINT_IF_VERBOSE ("done\n");

      PRINT_IF_VERBOSE ("...verify address 0x%04X: ", add);
      if (word == Stlink_Read_Word (add)) {
        printf ("done\n");
      } else {
        printf ("failed!\n");
        exit (EXIT_FAILURE);
      }
    } else if ( !strcasecmp(argv[i], "-rb") ) {
      int add;
      uint32_t byte;

      i++;
      if ( (sscanf(argv[i], "%i", &add) != 1) || (add > 0xFFFFFF) ) {
        printf ("Wrong -rb argument address parameter! Aborted\n");
        exit (EXIT_FAILURE);
      }
      PRINT_IF_VERBOSE ("...reading address 0x%04X: ", add);
      byte = Stlink_Read_Byte (add);
      if (prog_mode & PROG_MODE_VERBOSE)
        printf ("0x%02X (%d)\n", byte, byte);
      else
        printf ("0x%02X (%d)\n", byte, byte);
    } else if ( !strcasecmp(argv[i], "-rw") ) {
      int add;
      uint32_t word;

      i++;
      if ( (sscanf(argv[i], "%i", &add) != 1) || (add > 0xFFFFFF) ) {
        printf ("Wrong -rb argument address parameter! Aborted\n");
        exit (EXIT_FAILURE);
      }
      PRINT_IF_VERBOSE ("...reading address 0x%04X: ", add);
      word = Stlink_Read_Word (add);
      if (prog_mode & PROG_MODE_VERBOSE)
        printf ("0x%04X (%d)\n", word, word);
      else
        printf ("0x%04X (%d)\n", word, word);
    } else if ( !strcasecmp(argv[i], "-ib") ) {
      int add;
      uint32_t byte;

      i++;
      if ( (sscanf(argv[i], "%i", &add) != 1) || (add > 0xFFFFFF) ) {
        printf ("Wrong -ib argument address parameter! Aborted\n");
        exit (EXIT_FAILURE);
      }
      
      PRINT_IF_VERBOSE ("...reading address 0x%04X: ", add);
      byte = Stlink_Read_Byte (add);
      PRINT_IF_VERBOSE ("0x%02X\n", byte);

      byte++;
      byte &= 0xFF;
      PRINT_IF_VERBOSE ("...writing back 0x%02X to address 0x%04X: ", byte, add);
      Stlink_Unlock_Memory (&uc, add);
      Stlink_Prog_Byte (add, byte);
      PRINT_IF_VERBOSE ("done\n");
      
      PRINT_IF_VERBOSE ("...verify address 0x%04X: ", add);
      if (byte == Stlink_Read_Byte (add)) {
        if (prog_mode & PROG_MODE_VERBOSE)
          printf ("success\n");
        else
          printf ("Incremented address 0x%04X to 0x%02X (%d)\n",
              add, byte, byte);
      } else {
        if (prog_mode & PROG_MODE_VERBOSE)
          printf ("failed\n");
        else
          printf ("Increment address 0x%04X: byte verification failed!\n", add);
        exit (EXIT_FAILURE);
      }
    } else if ( !strcasecmp(argv[i], "-iw") ) {
      int add;
      uint32_t word;

      i++;
      if ( (sscanf(argv[i], "%i", &add) != 1) || (add > 0xFFFFFF) ) {
        printf ("Wrong -iw argument address parameter! Aborted\n");
        exit (EXIT_FAILURE);
      }
      
      PRINT_IF_VERBOSE ("...reading address 0x%04X: ", add);
      word = Stlink_Read_Word (add);
      PRINT_IF_VERBOSE ("0x%04X\n", word);

      word++;
      word &= 0xFFFF;
      PRINT_IF_VERBOSE ("...writing back 0x%04X to address 0x%04X: ", word, add);
      Stlink_Unlock_Memory (&uc, add);
      Stlink_Prog_Byte (add+1, word & 0xFF);
      if (!(word & 0xFF))
        Stlink_Prog_Byte (add, word>>8);
      PRINT_IF_VERBOSE ("done\n");

      PRINT_IF_VERBOSE ("...verify address 0x%04X: ", add);
      if (word == Stlink_Read_Word (add)) {
        if (prog_mode & PROG_MODE_VERBOSE)
          printf ("success\n");
        else
          printf ("Incremented address 0x%04X to 0x%04X (%d)\n",
              add, word, word);
      } else {
        if (prog_mode & PROG_MODE_VERBOSE)
          printf ("failed\n");
        else
          printf ("Increment address 0x%04X: Word verification failed!\n", add);
        exit (EXIT_FAILURE);
      }
    }
  }

//if memory is unlocked, lock back
  if (prog_stat & (PROG_STAT_UL_EEPROM | PROG_STAT_UL_FLASH)) {
    uint32_t iaspr;

    (prog_mode & PROG_MODE_STM8L) ? (iaspr = 0x5054) : (iaspr = 0x505F);
    Stlink_Write_Byte (iaspr, 0x00);
  }

//release CPU
/*
  Stlink_Write_Byte (0x7F80, 0x00);
  Stlink_Write_Byte (STM8_DM_CSR2, 0x00);
  Stlink_Swim_Cmd (STLINK_SWIM_RESET);
*/

//reset device
  Stlink_Swim_Cmd (STLINK_SWIM_GEN_RST);
  if (stlink_wait_swim_idle ()) {
    printf ("Error, µC reset: SWIM status not idle\n");
    exit (EXIT_FAILURE);
  }


  return 0;
}
