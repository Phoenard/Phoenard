/*
The MIT License (MIT)

This file is part of the Phoenard Arduino library
Copyright (c) 2014 Phoenard

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "PHNSDMinimal.h"

uint8_t card_notSDHCBlockShift;         /* Card is SD1 or SD2, and NOT SDHC. In that case this value is 9, 0 otherwise */

union SDMINFAT::cache_t volume_cacheBuffer;      /* 512 byte cache for device blocks */
uint32_t volume_cacheBlockNumber;       /* Logical number of block in the cache */
uint8_t  volume_cacheDirty = 0;         /* readCache() will write current block first if true */
uint8_t  volume_cacheFATMirror = 0;     /* current block in cache is a mirrored FAT block */
CardVolume volume;                      /* stores all current volume information */

uint8_t   file_isroot16dir;             /* file is a FAT16 root directory */
uint32_t  file_curCluster;              /* cluster for current file position */
uint32_t  file_position;                /* current file position in bytes from beginning */
FilePtr   file_curDir;                  /* directory currently selected */
uint32_t  file_size;                    /* total size of the currently opened file */
/* ============================================================================== */

/* Macro to send a byte to SPI */
#define spiWait()   while (!(SPSR & (1 << SPIF)));
#define spiSend(b)  SPDR = b; spiWait();

/* Receive a byte from SPI. Can be used to replace spiSend(0xFF) */
static uint8_t spiRec(void) {
  spiSend(0XFF);
  return SPDR;
}

/* Macro to skip some received bytes; used for inlining */
#define spiSkip_inline(count)  ({ \
  uint8_t cnt = count; \
  do { \
    spiRec(); \
  } while (--cnt); \
})

/* Skipping received bytes using successive calls to spiRec() */
static void spiSkip(uint8_t count) {
  spiSkip_inline(count);
}

/* send command and return error code.  Return zero for OK */
uint8_t card_command(uint8_t cmd, uint32_t arg, uint8_t crc) {
  uint8_t cnt, result;
  CardCommand command;
  uint8_t* command_data_end;
  uint8_t* command_data;

  /* Wait until no longer busy */
  /* Unused, does not appear to be required */
  /* card_waitForData(DATA_IDLE_BLOCK); */

  /* 
   * Write out the full command
   * First all arguments are packed into a single command struct
   * Then the entire struct is written out in one go
   */
  command.cmd = cmd | 0x40;
  command.arg = arg;
  command.crc = crc;
  command_data_end = (uint8_t*) &command;
  command_data = command_data_end + sizeof(CardCommand);
  do {
    spiSend(*(--command_data));
  } while (command_data != command_data_end);

  /* wait for response and return it, up till 256 retries, then fail */
  cnt = 0;
  do {
    result = spiRec();
  } while ((result & 0x80) && ++cnt);

  return result;
}

/* wait for spiRec() to return a specific value, with ~200ms timeout */
uint8_t card_waitForData(uint8_t data_state) {
  /* Maximum of 0xFFFF cycles is about 200 ms */
  for (uint16_t i = 0xFFFF; i; i--) {
    if (spiRec() == data_state) return 1;
  }
  return 0;
}

/* Turns chip-select on/off, needed when communicating with other SPI devices */
void card_setEnabled(uint8_t enabled) {
  /* Don't do anything if already enabled */
  if (!(SD_CS_PORT & SD_CS_MASK) && enabled) {
    return;
  }

  /* Disable the chip-select */
  SD_CS_PORT |= SD_CS_MASK;

  /* Wait for ~74 clock cycles, then set chip-select back LOW */
  if (enabled) {
    /* Note: inlined because using spiSkip negatively impacts bootloader size */
    spiSkip_inline(10);
    SD_CS_PORT &= ~SD_CS_MASK;
  }
}

/* cache a file's directory entry
 * return pointer to cached entry */
SDMINFAT::dir_t* file_readCacheDir(void) {
  volume_readCache(file_curDir.block);
  return volume_cacheBuffer.dir + file_curDir.index;
}

/* Updates the current cache block number without actually reading in the data */
uint8_t volume_updateCache(uint32_t blockNumber) {
  if (volume_cacheBlockNumber == blockNumber) {
    return 0;
  } else {
    /* Flush the cache if dirty */
    if (volume_cacheDirty) {
      volume_writeCache();
    }
  
    /* Update the cache block number after potentially writing out */
    volume_cacheBlockNumber = blockNumber;
  
    return 1;
  }
}

/* 
 * Reads the block specified into the cache if it is not already loaded
 * If the current cache data still needs to be written out, this is done first
 */
void volume_readCache(uint32_t blockNumber) {
  uint8_t* data = volume_cacheBuffer.data-1;
  uint8_t* data_end = volume_cacheBuffer.data + 512;
  if (volume_updateCache(blockNumber)) {
    /* Use address if not SDHC card */
    if (card_command(SDMINFAT::CMD17, blockNumber << card_notSDHCBlockShift, 0XFF)) goto fail;
    if (!card_waitForData(SDMINFAT::DATA_START_BLOCK)) goto fail;

    /* 
     * Read the data one byte at a time
     * Read one extra byte at the end which is discarded
     */
    for (;;) {
      SPDR = 0xFF;
      data++;
      spiWait();
      if (data == data_end) break;
      *data = SPDR;
    }
  }
  return;
  
fail:
  volume.isInitialized = 0;
  return;
}

/*
 * Writes the current data in cache to the SD-card
 * This function always writes, even if the cache is left unchanged since the last read
 */
void volume_writeCache() {
  /* Only write if current cache is NOT the root directory - safety check */
  if (volume_cacheBlockNumber) {
    volume_writeCache(volume_cacheBlockNumber);
  }
}

/*
 * Writes the current data in cache to the SD-card
 * This function always writes, even if the cache is left unchanged since the last read
 * With the block parameter one can specify where to write the cache to.
 * The cache block number is not changed
 * Note that this function allows writing to the zero-block, so be careful!
 */
void volume_writeCache(uint32_t block) {
  uint8_t* data;
  uint8_t* data_end = volume_cacheBuffer.data + 512;
  volume_cacheDirty = 0;
  volume_cacheBlockNumber = block;

  /* don't write anything, ever, if volume is not initialized */
  if (!volume.isInitialized) return;

write_block:

  /* don't allow write to first block - TODO: Can this be removed? */
  if (block == 0) return;

  /* use address if not SDHC card */
  if (card_command(SDMINFAT::CMD24, block << card_notSDHCBlockShift, 0XFF)) goto fail;
  spiSend(SDMINFAT::DATA_START_BLOCK);

  /* Write data from buffer to SPI - optimized loop */
  data = volume_cacheBuffer.data;
  do {
    SPDR = *data;
    data++;
    spiWait();
  } while (data != data_end);

  spiRec();    /* dummy crc */
  spiRec();    /* dummy crc */

  /* Wait for programming of flash to complete */
  if ((spiRec() & SDMINFAT::DATA_RES_MASK) != SDMINFAT::DATA_RES_ACCEPTED) goto fail;
  if (!card_waitForData(SDMINFAT::DATA_IDLE_BLOCK)) goto fail;
  if (card_command(SDMINFAT::CMD13, 0, 0XFF)) goto fail;
  if (spiRec()) goto fail;

  /* Also write out a mirror block if specified */
  if (volume_cacheFATMirror) {
    volume_cacheFATMirror = 0;
    block += volume.blocksPerFat;
    goto write_block;
  }
  return;
  
fail:
  volume.isInitialized = 0;
}

static void volume_fatLoad(uint32_t cluster) {
  /* If the cluster is outside the volume, that is an error */
  if (cluster > volume.clusterLast) volume.isInitialized = 0;
  
  /* Read the FAT Block into the cache */
  volume_readCache(volume.fatStartBlock + (cluster >> (7 + volume.isfat16)));
  
  /* If multiple FAT, mark current block in cache mirrored */
  volume_cacheFATMirror = volume.isMultiFat;
}

/* Fetch a FAT entry, returns True when successful, False when the cluster is an EOC */
uint8_t volume_fatGet(uint32_t cluster, uint32_t* value) {
  /* If an error occurred, assume end of chain.
   * This ensures that most while loops used for chaining break free
   * Then check if the cluster is the end of the chain, and if so, indicate that it is */
  if (!volume.isInitialized || cluster >= (volume.isfat16 ? SDMINFAT::FAT16EOC_MIN : SDMINFAT::FAT32EOC_MIN)) return 0;

  /* Cache the block and return the value found there */
  volume_fatLoad(cluster);
  if (volume.isfat16) {
    *value = volume_cacheBuffer.fat16[cluster & 0XFF];
  } else {
    *value = volume_cacheBuffer.fat32[cluster & 0X7F] & SDMINFAT::FAT32MASK;
  }
  return 1;
}

/* Store a FAT entry */
void volume_fatPut(uint32_t cluster, uint32_t value) {
  /* do not put if reserved cluster */
  if (cluster < 2) return;

  /* calculate block address for entry */
  volume_fatLoad(cluster);

  /* store entry */
  if (volume.isfat16) {
    volume_cacheBuffer.fat16[cluster & 0XFF] = value;
  } else {
    volume_cacheBuffer.fat32[cluster & 0X7F] = value;
  }

  /* Mark current cache dirty since we wrote to it */
  volume_cacheDirty = 1;
}

/* Ensure card is initialized by opening an arbitrary (non-existent) file */
uint8_t volume_init(uint8_t resetPosition) {
  const char name_none[1] = {0};
  file_open(name_none, name_none, FILE_READ);
  if (!volume.isInitialized) return 0;
  
  /* Prepare for reading the root directory entries */
  if (resetPosition) {
    file_position = 0;
    file_curCluster = volume.rootCluster;
    file_isroot16dir = volume.isfat16;
  }
  return 1;
}

/**
 * The flush() call causes all modified data and directory fields
 * to be written to the storage device. With save, the file name
 * to write to can be specified as well
 */
void file_save(char filename[8]) {
  SDMINFAT::dir_t* p = file_readCacheDir();

  /* update file size */
  p->fileSize = file_size;

  /* update first cluster fields */
  memcpy(p->name, filename, 8);
  volume_writeCache();
}

/* Frees the file content clusters and frees the directory entry of the current file */
/* Note: MUST be performed right after opening a file, do not write/read! */
void file_delete(void) {
  /* Delete contents of file */
  file_truncate();
  
  /* Set first char of filename to mark deletion, then save */
  char filename[8];
  filename[0] = SDMINFAT::DIR_NAME_DELETED;
  file_save(filename);
}

/**
 * The flush() call causes all modified data and directory fields
 * to be written to the storage device.
 */
void file_flush(void) {
  SDMINFAT::dir_t* p = file_readCacheDir();

  /* update file size */
  p->fileSize = file_size;

  /* update first cluster fields */
  volume_writeCache();
}

uint8_t file_open(const char* filename, const char* ext, uint8_t mode) {
  /* Variables used down below */
  const int filename_83fmt_len = 11;
  unsigned char filename_83fmt[filename_83fmt_len];
  uint16_t timeout_cycle_ctr = 0;
  uint32_t capacity_arg;
  uint8_t fi;

  /* Copy filename and file extension to the combined 83format filename */
  memcpy(filename_83fmt, filename, 8);
  memcpy(filename_83fmt + 8, ext, 3);

  /* If card/volume is not initialized, initialize it */
  if (!volume.isInitialized) {

    /* Initialize SPI port */
    SPI_DDR = (SPI_DDR & ~SPI_MASK) | SPI_INIT_DDR;
    SPI_PORT = (SPI_PORT & ~SPI_MASK) | SPI_INIT_PORT;

    /* Initialize chip select port */
    SD_CS_DDR |= SD_CS_MASK;
    SD_CS_PORT |= SD_CS_MASK;

    /* Enable SPI, Master, clock rate f_osc/128 */
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);

    /* Set Sck Rate to half */
    SPSR &= ~((1 << SPI2X) | (1 <<SPR1) | (1 << SPR0));

    /* must supply min of 74 clock cycles with CS high. */
    spiSkip(10);

    /* CHIP SELECT of the card LOW indefinitely */
    SD_CS_PORT &= ~SD_CS_MASK;

    /* command to go idle in SPI mode */
    while (card_command(SDMINFAT::CMD0, 0, 0X95) != SDMINFAT::R1_IDLE_STATE) {

      /* check for timeout */
      if (timeout_cycle_ctr++ & 32) return 0;  
    }

    /* check SD version */
    if ((card_command(SDMINFAT::CMD8, 0x1AA, 0X87) & SDMINFAT::R1_ILLEGAL_COMMAND)) {
      /* Card type: SD1 */
      capacity_arg = 0X00000000;
      card_notSDHCBlockShift = 9; /* SD1 Is never a SDHC card */
    } else {
      /* Card type: SD2
       * only need last byte of r7 response
       * Skip first 3 bytes */
      spiSkip(3);
      if (spiRec() != 0XAA) return 0;
      capacity_arg = 0X40000000;
      card_notSDHCBlockShift = 0; /* SD2 CAN be a SDHC card */
    }

    /* Execute CMD55/ACMD41 command until ready state is achieved */
    for (;;) {
      card_command(SDMINFAT::CMD55, 0, 0XFF);
      if (card_command(SDMINFAT::ACMD41, capacity_arg, 0XFF) == SDMINFAT::R1_READY_STATE) break;
    
      /* check for timeout */
      if (timeout_cycle_ctr++ & 128) return 0;  
    }

    /* if SD2 (and NOT SDHC is 0) read OCR register to check for SDHC card */
    if (!card_notSDHCBlockShift) {
      if (card_command(SDMINFAT::CMD58, 0, 0XFF)) return 0;
      if ((spiRec() & 0XC0) != 0XC0) card_notSDHCBlockShift = 9;

      /* discard rest of ocr - contains allowed voltage range */
      spiSkip(3);
    }

    /* Set SPI SCK Speed */
#if (SCK_SPEED & 0x1) || (SCK_SPEED == 6)
    SPSR &= ~(1 << SPI2X);
#else
    SPSR |= (1 << SPI2X);
#endif
    SPCR &= ~((1 <<SPR1) | (1 << SPR0));
    SPCR |= (SCK_SPEED & 4 ? (1 << SPR1) : 0) | (SCK_SPEED & 2 ? (1 << SPR0) : 0);

    /* Initialize an available partition volume, first try 1, then 0 */
    for (uint8_t part = 1;; part--) {
      /* If no partition at 1 or 0, we failed */
      if (part == 0xFF) return 0;

      uint32_t volumeStartBlock = 0;

      /* if part == 0 assume super floppy with FAT boot sector in block zero
       * if part > 0 assume mbr volume with partition table */
      volume_cacheBlockNumber = 0XFFFFFFFF;
      volume_readCache(0);
      if (part) {
        SDMINFAT::part_t* p = &volume_cacheBuffer.mbr.part[part-1];
      
        /* not a valid partition? */
        if ((p->boot & 0X7F) !=0) continue;
        if (p->totalSectors < 100) continue;
        if (p->firstSector == 0) continue;
      
        volume_readCache(volumeStartBlock = p->firstSector);
      }
      SDMINFAT::bpb_t* bpb = &volume_cacheBuffer.fbs.bpb;
    
      /* not valid FAT volume? */
      if (bpb->bytesPerSector != 512) continue;
      if (bpb->fatCount == 0) continue;
      if (bpb->reservedSectorCount == 0) continue;
      if (bpb->sectorsPerCluster == 0) continue;

      volume.isMultiFat = bpb->fatCount > 1;
      volume.blocksPerCluster = bpb->sectorsPerCluster;
      volume.blocksPerFat = bpb->sectorsPerFat16 ? bpb->sectorsPerFat16 : bpb->sectorsPerFat32;

      volume.fatStartBlock = volumeStartBlock + bpb->reservedSectorCount;

      /* directory start for FAT16 dataStart for FAT32 */
      volume.rootCluster = volume.fatStartBlock + bpb->fatCount * volume.blocksPerFat;

      /* total blocks for FAT16 or FAT32 */
      volume.clusterLast = bpb->totalSectors16 ? bpb->totalSectors16 : bpb->totalSectors32;
      /* data start for FAT16 and FAT32 */
      volume.rootSize = 32 * bpb->rootDirEntryCount;
      volume.dataStartBlock = volume.rootCluster + ((volume.rootSize + 511)/512);
      /* Total data blocks */
      volume.clusterLast -= (volume.dataStartBlock - volumeStartBlock);
      /* divide by cluster size to get cluster count */
      volume.clusterLast /= volume.blocksPerCluster;
      /* Incremented to make usage logic easier to use */
      volume.clusterLast++;

      /* We do not support FAT 12 */
      if (volume.clusterLast <= 4085) continue;

      /* FAT type is determined by cluster count */
      volume.isfat16 = (volume.clusterLast <= 65525);
      if (!volume.isfat16) {  
        volume.rootCluster = bpb->fat32RootCluster;
      }

      /* Success! */
      volume.isInitialized = 1;
      break;
    }
  } /* Initialization end */

  /* Validate the file name - this prevents SD corruption issues */
  fi = filename_83fmt_len - 1;
  do {
    if (filename_83fmt[fi] & 0x80) return 0;
    if (filename_83fmt[fi] < 32) return 0;
  } while (fi--);

  /* set to root directory */
  file_position = 0;
  file_isroot16dir = volume.isfat16;
  file_curCluster = volume.rootCluster;

  /* Locate this file on the root volume */
  SDMINFAT::dir_t* p;
  uint8_t index = 0xF;
  uint8_t entryFound = 0;
  uint8_t fileFound = 0;
  do {
    /* Read sectors of 32 bytes, when it reaches the final sector (16) go to the next block */
    index++;
    index &= 0xF;

    /* Move the reader to the next 32 bytes, once it reaches 512 the next block is read */
    p = (SDMINFAT::dir_t*) file_read(32);

    /* Is this our file? */
    fileFound = (!memcmp(filename_83fmt, p->name, 11) && p->isFile());

    char c = p->name[0];
    if (fileFound || (c == SDMINFAT::DIR_NAME_FREE) || (c == SDMINFAT::DIR_NAME_DELETED)) {
      /* Store found entry block and index */
      if (!entryFound || fileFound) {
        entryFound = 1;
        file_curDir.block = volume_cacheBlockNumber;
        file_curDir.index = index;
      }
      /* Check for completion or end of directory */
      if (c == SDMINFAT::DIR_NAME_FREE) break;
    }
  } while (volume.isInitialized && !fileFound);

  /* Create a new file entry */
  if (!fileFound) {
    /* only create new files when CREATING is set */
    if (!(mode & FILE_CREATE)) return 0;
    /* can not create new root clusters on FAT16 */
    if (!entryFound && volume.isfat16) return 0;

    /* No usable entry was found - append a new cluster root cluster */
    if (!entryFound) {
      /* Calling cacheCurrentBlock appends a new cluster to the chain */
      /* Note: cache is not dirty after this call, we can safely discard it! */
      file_position = 0;
      volume_cacheCurrentBlock(1);

      /* use first entry in cluster */
      file_curDir.block = volume_cacheBlockNumber;
      file_curDir.index = 0;

      /* Fill cache with zero data */
      /* Loop takes less flash than memset(volume_cacheBuffer_.data, 0, 512); */
      for (uint16_t d = 0; d < 512; d++) {
        volume_cacheBuffer.data[d] = 0;
      }

      /* Write this zero cache to all the clusters */
      uint8_t i = volume.blocksPerCluster;
      do {
        volume_writeCache();
        volume_cacheBlockNumber++;
      } while (--i);
    }
    p = file_readCacheDir();

    /* initialize as empty file */
    memset(p, 0, sizeof(SDMINFAT::dir_t));
    memcpy(p->name, filename_83fmt, 11);

    /* set timestamps */
    p->lastWriteDate = p->lastAccessDate = p->creationDate = SDMINFAT::FAT_DEFAULT_DATE;
    p->lastWriteTime = p->creationTime = SDMINFAT::FAT_DEFAULT_TIME;

    /* force write of entry to SD */
    volume_writeCache();

  } /* Entry creation end */

  /* write to a read-only file is an error */
  if (mode && p->isReadOnly()) return 0;

  /* Set up file fields to point to this file */
  file_curCluster = p->firstCluster();
  file_size = p->fileSize;
  file_isroot16dir = 0;
  file_position = 0;

  /* Writing to a non-empty file requires the file to be wiped first */
  if (file_size && (mode & FILE_WIPE)) {
    file_truncate();
  }

  return volume.isInitialized;
}

void file_truncate() {
  /* Delete contents of file */
  uint32_t next;
  uint32_t firstClst = file_curCluster;
  while (volume_fatGet(firstClst, &next)) {
    volume_fatPut(firstClst, 0);
    firstClst = next;
  }
  file_curCluster = 0;
  file_size = 0;
  
  /* Update file entry to show as empty to prevent corrupted state */
  SDMINFAT::dir_t* p = file_readCacheDir();
  p->fileSize = 0;
  p->setFirstCluster(0);
  volume_writeCache();
}

/* Caches the block at the current position in the file
 * writeCluster indicates whether a new cluster can be appended */
uint8_t* volume_cacheCurrentBlock(uint8_t writeCluster) {
  uint16_t offset = (file_position & 0X1FF);
  uint32_t block = (file_position >> 9) & (volume.blocksPerCluster - 1);
  if (file_isroot16dir) {
    block += volume.rootCluster;
  } else {
    if (offset == 0 && block == 0) {
      if (writeCluster) {
        /* Add a new cluster (was file_addCluster()) */

        uint32_t cluster; /* Free cluster found */

        /*
         * Start looking from the current cluster
         * Note: curCluster can be 0 (<2), we rely on fatGet to return nonzero here
         */
        cluster = file_curCluster;

        /* search the FAT for one free cluster */
        uint32_t linkClst;
        for (;;) {
          /* Reached (the other way) around the start? Fail! */
          if (++cluster == file_curCluster) {
            volume.isInitialized = 0;
            goto eof;
          }

          /* Past end - start from beginning of FAT */
          if (cluster > volume.clusterLast) {
            cluster = 2;
          }

          /* Read the next cluster of this cluster, if 0 it is free */
          volume_fatGet(cluster, &linkClst);
          if (linkClst == 0) {
            /* Free, break and use this cluster */
            break;
          }
        }

        /* mark found cluster end of chain */
        volume_fatPut(cluster, 0x0FFFFFFF);

        if (file_curCluster == 0) {
          /* first cluster; write information to file dir block */
          SDMINFAT::dir_t* p = file_readCacheDir();
          p->setFirstCluster(cluster);
          volume_writeCache();
        } else {
          /* connect chains */
          volume_fatPut(file_curCluster, cluster);
        }

        /* update the current cluster to the last added one */
        file_curCluster = cluster;
      } else if (file_position > 0) {
        /* get next cluster from FAT */
        volume_fatGet(file_curCluster, &file_curCluster);
      }
    }

    /* Add the block index of the start of the current cluster */
    block += volume_firstClusterBlock(file_curCluster);
  }
  if (writeCluster) {
    /* Flush anything in the buffer right now; proceed to write */
    volume_updateCache(block);
  } else {
    /* Read in the new block */
    volume_readCache(block);
  }
eof:
  return volume_cacheBuffer.data + offset;
}

/* 
 * Read the next section of data.
 * The position must be incremented with the same values during a single run
 * and no more than 128, the increment being a factor of 2 (1,2,4,8,16,32,64,128)
 * 
 * Using variable increments is possible; as long as you make sure your modulus is 512.
 * Not doing so results in the current cluster not getting updated.
 *
 * It is not recommended to use file_read_byte() and file_read() interchangeably.
 */
char* file_read(uint16_t nbyteIncrement) {
  /* read block to cache and copy data to caller */
  char* read = (char*) volume_cacheCurrentBlock(0);

  /* Increment position read */
  file_position += nbyteIncrement;
  return read;
}

void file_write(const char* data, uint16_t nBytes) {
  memcpy(volume_cacheCurrentBlock(1), data, nBytes);
  volume_cacheDirty = 1;

  /* Increment position read */
  file_position += nBytes;
  if (file_position > file_size) file_size = file_position;
}

/* 
 * Writes a single byte to the file.
 */
void file_write_byte(char b) {
  /* Cache the current block and set the byte in that buffer */
  volume_cacheCurrentBlock(1)[0] = b;
  volume_cacheDirty = 1;

  /* Increment the bytes written */
  file_position++;
  if (file_position > file_size) file_size = file_position;
}

/* 
 * Read a single byte and increment the read position by one
 * It is not recommended to use file_read_byte() and file_read() interchangeably.
 */
char file_read_byte(void) {
  return *file_read(1);
}

/* 
 * Writes a single byte to the file. Taken over from the original buffer writing,
 * optimized to write single bytes at a time only
 * Note that this function is for appending bytes only.
 * It is assumed that the file position == file size.
 */
void file_append_byte(char b) {
  /* Cache the current block and set the byte in that buffer */
  volume_cacheCurrentBlock(1)[0] = b;
  volume_cacheDirty = 1;

  /* Increment the bytes written */
  file_position = ++file_size;
}

/* Reads a single line of HEX data from a HEX file */
uint8_t file_read_hex_line(uint8_t* buff) {
  uint8_t length = 0;
  uint8_t found_data_start = 0;
  while (file_position < file_size) {
    /* Read the next byte, handle block incrementing */
    char c = *file_read(1);
    if (found_data_start) {
      /* Read the data until a char before the 0-range is read */
      if (c < '0') break;
      
      /* Start of a new byte, set to an initial 0x0 */
      if (!(length & 0x1)) *buff = 0x0;
      
      /* Convert the character into HEX, put it into the buffer */
      *buff <<= 4;
      if (c & 0x40) {
        c -= ('A' - '0') - 10;
      }
      *buff += (c - '0') & 0xF;
      if (length++ & 0x1) buff++;
    } else {
      /* Wait for : to be received */
      found_data_start = (c == ':');
    }
  }
  return length;
}

/* 
 * Writes a single line of HEX data to a HEX file. Takes care of CRC byte.
 * Make sure to leave 4 bytes available for the address, length and record type fields
 * A maximum of 123 bytes of data can be written per line
 */
void file_append_hex_line(uint8_t* buff, uint8_t len, uint32_t address, unsigned char recordType) {
  /* Append data at the first 4 bytes */
  buff[0] = len;
  buff[1] = (unsigned char) (address >> 8);
  buff[2] = (unsigned char) (address & 0xFF);
  buff[3] = recordType;
  len += 4;
  len <<= 1;

  /* Start actual writing */
  file_append_byte(':');
  unsigned int crc = 0;
  do {
    uint8_t data;
    if (len) {
      /* Did not reach end, write data byte and handle CRC */
      data = *(buff++);
      crc += data;
    } else {
      /* Reached end, write CRC */
      data = (~crc + 1) & 0xFF;
    }

    /*
     * Write the two HEX characters. The length is used for both
     * inner and outer loops to reduce code size. This leafe ons
     * more space for LCD/Bootloader logic!
     */
    do {
      unsigned char c = (data & 0xF0) >> 4;
      data <<= 4;

      if (c >= 10) {
        c += 'A' - '0' - 10;
      }
      c += '0';
      file_append_byte(c);
    } while (--len & 0x1);
  } while (len != 0xFE);
  
  file_append_byte('\r');
  file_append_byte('\n');
}