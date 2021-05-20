
static int
read_next_byte (unsigned char *buffer, int offset)
{
  int qh, ql;

  qh = *(buffer+offset);
  ql = *(buffer+offset+1);

  if ( qh<0x3A && qh>0x2F ) {
    qh -= 0x30;
  } else {
    switch (qh) {
    case 'f':
    case 'F':
      qh = 0xF;
      break;
    case 'e':
    case 'E':
      qh = 0xE;
      break;
    case 'd':
    case 'D':
      qh = 0xD;
      break;
    case 'c':
    case 'C':
      qh = 0xC;
      break;
    case 'b':
    case 'B':
      qh = 0xB;
      break;
    case 'a':
    case 'A':
      qh = 0xA;
      break;
    default:
      return -1;
    }
  }

  if ( ql<0x3A && ql>0x2F ) {
    ql -= 0x30;
  } else {
    switch (ql) {
    case 'f':
    case 'F':
      ql = 0xF;
      break;
    case 'e':
    case 'E':
      ql = 0xE;
      break;
    case 'd':
    case 'D':
      ql = 0xD;
      break;
    case 'c':
    case 'C':
      ql = 0xC;
      break;
    case 'b':
    case 'B':
      ql = 0xB;
      break;
    case 'a':
    case 'A':
      ql = 0xA;
      break;
    default:
      return -1;
    }
  }

  return (qh<<4)|ql;
}

static int
read_next_word (unsigned char *buffer, int offset)
{
  int h, l;

  h = read_next_byte (buffer, offset);
  if (h==-1)
    return -1;
  l = read_next_byte (buffer, offset + 2);
  if (l==-1)
    return -1;
  return (h<<8)|l;
}

/*
From wiki:
Record Format

An Intel HEX file is composed of any number of HEX records. Each record is made
up of five fields that are arranged in the following format:

:llaaaatt[dd...]cc

Each group of letters corresponds to a different field, and each letter
represents a single hexadecimal digit. Each field is composed of at least two
hexadecimal digits-which make up a byte-as described below:

    :     the colon that starts every Intel HEX record.
    ll     record-length field that represents the number of data bytes (dd)
      in the record.
    aaaa   address field that represents the starting address for subsequent
      data in the record
    tt     HEX record type, which may be one of the following:
    00 - data record
    01 - end-of-file record
    02 - extended segment address record
    04 - extended linear address record
    05 - start linear address record
    dd    data field that represents one byte of data. A record may have
      multiple data bytes. The number of data bytes in the record must
      match the number specified by the ll field.
    cc    checksum field that represents the checksum of the record,
      calculated by summing the values of all hexadecimal digit pairs in
      the record modulo 256 and taking the two's complement.
*/


/* Extended memory segment address is not handled in the fucntion!!!
 */
void
Ihex_Wr_Data (unsigned char *data, uint32_t address, uint32_t size, FILE *file)
{
  uint32_t lcnt, dcnt, chksum;

  dcnt = 0;
  while (size) {
    (size > 32) ? (lcnt=32) : (lcnt=size);
    fprintf (file, ":%02X%04X00", lcnt, address);
    chksum = lcnt + (address>>8) + (address & 0xFF);
    for (int i=0; i<lcnt; i++) {
      fprintf (file, "%02X", data[dcnt]);
      chksum += data[dcnt++];
    }
    fprintf (file, "%02X\n", (0x100 - chksum) & 0xFF);
    address += lcnt;
    size -= lcnt;
  }
}

/* La apelare pointerul trebuie sa fie la inceputul fisierului. Functia se
 * apeleaza dupa deschiderea fisierului si returneaza numarul de blocuri
 * definite in fisier.
 * block_add reprezinta adresa aliniata de start a blocului curent
 */
int
Ihex_Count_Blocks (FILE *file, int blk_size)
{
  unsigned char line[512+32];
  int off, line_index, line_sta, line_end;
  int block_add, block_index;

  line_index  = 0;
  off = 0;
  block_index = 0;
  block_add = 0;

  while ( fgets((char *)line, sizeof(line), file) ) {
    line_index++;
    if (line[0]!=':')
      goto file_err;
    //read the number of data bytes in the record
    line_end = read_next_byte (line, 1);
    if (line_end==-1)
      goto file_err;
    //read the address of the record
    line_sta = read_next_word (line, 3);
    if (line_sta==-1)
      goto file_err;
    line_sta += off;
    line_end += line_sta;
    //read the record type
    switch (read_next_byte (line, 7)) {
    case -1:
      goto file_err;
    case 0x00:
    //data record
      if (line_sta >= (block_add + blk_size)) {
        block_add = line_sta & ~(blk_size - 1);
        block_index++;
      }
      while (line_end > (block_add + blk_size)) {
        block_add += blk_size;
        block_index++;
      }
      break;
    case 0x01:
    //end of file record
      break;
    case 0x02:
    //extended address
      off = read_next_word (line, 9);
      if (off==-1)
        goto file_err;
      off *= 16;
      break;
    default:
      printf (
	      "Not supported record type found in %s file, line no. %d\n",
          ghexfile_name, line_index);
      exit (EXIT_FAILURE);
    }
  }
  return block_index;

file_err:
  printf ("Error in %s file, line no. %d, not an intel hex file?\n",
      ghexfile_name, line_index);
  exit (EXIT_FAILURE);
}


/* La apelare *data si *ddef trebuie sa fie resetate, functia scrie in *data
 * doar octetii definiti in fisier, iar in *ddef se indica octetii definiti
 * cu valoarea 0xFF pentru a se putea folosi ca mask la programarea selectiva
 */
void
Ihex_Read_Data_Blocks (FILE *file, int blk_size, uint32_t *blk_ads,
    unsigned char *data, unsigned char *ddef)
{
  unsigned char line[512+32];
  int  off, i;
  int  line_index, line_cnt, line_add, line_rec;
  int  block_add, block_index;
  uint32_t checksum;

  line_index = 0;
  off = 0;
  block_index = 0;
  block_add = 0;

  fseek (file, 0, SEEK_SET);

  while ( fgets((char *)line, sizeof(line), file) ) {
    line_index++;
    //read the number of data bytes in the record
    i = 1;
    line_cnt = read_next_byte (line, i);
    i += 2;
    //read the address of the record
    line_add = read_next_word (line, i);
    i += 4;
    checksum = line_cnt + (line_add>>8) + (line_add & 0xFF);
    line_add += off;
    //read the record type
    line_rec = read_next_byte (line, i);
    i += 2;
    checksum += line_rec;
    switch (line_rec) {
    case 0x00:
    //data record
      if (line_cnt==0)
        goto file_err;
      if (line_add >= (block_add + blk_size)) {
        block_add = line_add & ~(blk_size - 1);
        *(blk_ads + block_index) = block_add;
        block_index++;
      }
      for (int j=0, q; j<line_cnt; j++) {
        if ((line_add + j) >= (block_add + blk_size)) {
          block_add += blk_size;
          *(blk_ads + block_index) = block_add;
          block_index++;
        }
        q = read_next_byte (line, i);
        if (q==-1)
          goto file_err;
        checksum += q;
        i += 2;
        uint32_t po = (block_index-1)*blk_size + line_add - block_add + j;
        *(data + po) = q;
        *(ddef + po) = 0xFF;
      }
      break;
    case 0x01:
    //end of file record
      if (line_cnt)
        goto file_err;
      break;
    case 0x02:
    //extended address
      if (line_cnt != 2)
        goto file_err;
      off = read_next_word (line, 9);
      off *= 16;
      break;
    default:
      printf ("Not supported record type found in %s file, line no. %d\n",
          ghexfile_name, line_index);
      exit (EXIT_FAILURE);
    }
    //read the checksum end byte
    checksum += read_next_byte (line, i);
    if (checksum & 0xFF) {
      printf ("Checksum error in %s intel hex file, line no. %d!\n",
          ghexfile_name, line_index);
      exit (EXIT_FAILURE);
    }
  }
  return;

file_err:
  printf ("Error in %s file, line no. %d, not an intel hex file?\n",
      ghexfile_name, line_index);
  exit (EXIT_FAILURE);
}
