
void Ihex_Wr_Data (unsigned char *data, uint32_t address, uint32_t size,
    FILE *file);
int  Ihex_Count_Blocks (FILE *file, int blk_size);
void Ihex_Read_Data_Blocks (FILE *file, int blk_size, uint32_t *blk_ads,
    unsigned char *data, unsigned char *ddef);
