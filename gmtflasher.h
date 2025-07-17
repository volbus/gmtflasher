
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libusb.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>

/*----------------------------------------------------------------------------*/
/* Local headers */

#include "stlink.h"
#include "ihex.h"
#include "version.h"


/*----------------------------------------------------------------------------*/
/* Macros & defines */

#define PRINT_IF_VERBOSE(...) do {		\
  if (prog_mode & PROG_MODE_VERBOSE)		\
    printf (__VA_ARGS__);			\
} while (0)

#define MALLOC_TST(ptr) do {						\
  if (!ptr) {								\
    printf ("%s:%s:%i: %s\n", __FILE__, __func__, __LINE__,		\
       strerror(errno));						\
    exit (EXIT_FAILURE);						\
  }									\
} while (0)


typedef struct {
  char     name[64];
  uint32_t flash_size;
  uint32_t eeprom_size;
  uint32_t eeprom_add;
  uint32_t block_size;
  uint32_t add_0;
  uint32_t add_1;
} mcu;

#define JOB_WRITE_ALL			0x000001
#define JOB_READ_ALL			0x000002
#define JOB_WRITE_FLASH			0x000004
#define JOB_READ_FLASH			0x000008
#define JOB_WRITE_EEPROM		0x000010
#define JOB_READ_EEPROM			0x000020
#define JOB_WRITE_OPT			0x000040
#define JOB_READ_OPT			0x000080
#define JOB_UNLOCK			0x000100
#define JOB_LOCK			0x000200
#define JOB_WRITE_BYTE			0x000400
#define JOB_READ_BYTE			0x000800
#define JOB_WRITE_WORD			0x001000
#define JOB_READ_WORD			0x002000
#define JOB_INC_BYTE			0x004000
#define JOB_INC_WORD			0x008000
#define JOB_READ_RANGE			0x010000
#define JOB_PRINT			0x020000
/*----------------------------------------------------------------------------*/
/* Globals */

char *ghexfile_name;
char *ofile_name;
FILE *ghexfile;

uint32_t prog_stat;
  #define PROG_STAT_UL_FLASH		0x0001
  #define PROG_STAT_UL_EEPROM		0x0002
  #define PROG_STAT_OFILE		0x0004
uint32_t prog_mode;
  #define PROG_MODE_VERBOSE		0x0001
  #define PROG_MODE_STM8L		0x0002
  #define PROG_MODE_FORCE_ALL		0x0004
  #define PROG_MODE_PERSIST		0x0008

/*----------------------------------------------------------------------------*/
/* Project source files */

#include "xml.c"
#include "stlink.c"
#include "ihex.c"
