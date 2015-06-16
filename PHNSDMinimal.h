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

/**
 * @file
 * @brief A minimal Micro-SD library implementation that focuses on minimal code size
 *
 * Also used by the bootloader, this minimalistic library has the `least` of the least.
 * It is intended to be used for just reading files, writing files and listing files.
 * File names are all in the 8.3 format and no fancy checks are performed. If something
 * goes wrong, the library is locked down to prevent damages - simple as that.
 *
 * If you want to do complex things like traversing directories - it is possible - but
 * at a cost. There will not be a fancy class like the SD File to do it, and you will
 * need to store the pointers in memory yourself.
 *
 * That said, this minimalistic library is truly intended for unique applications.
 * For one, the sketch list has to be minimal size to boost the loading times, so it
 * uses the minimal SD library instead.
 *
 * If you wish to dive deep into the FAT16 and FAT32 file systems, then this is your chance.
 * You are not prohibited to cross the lines...but be aware that mistakes will corrupt
 * your micro-sd card requiring it to be re-formatted. Please don't 'play' around with this
 * library on a card containing information you don't want to lose.
 *
 * Remember: it takes a single incorrect block write to permanently corrupt an entire directory.
 */
 
#ifndef SDMINIMAL_H_
#define SDMINIMAL_H_

#include <string.h>
#include <avr/pgmspace.h>
#include "PHNSDMinimal_fat.h"
#include "PHNCore.h"

#define SDMIN_FILE_READ    0
#define SDMIN_FILE_WRITE   (FILE_CREATE | FILE_WIPE)
#define SDMIN_FILE_CREATE  1
#define SDMIN_FILE_WIPE    2

/* SCK Speed defines */
/* Speed number, 0 is full speed, 6 is slowest speed */
#define SCK_SPEED 1

/* ============================================================================== */

/// Stores the block and index of a file dir entry
typedef struct {
  uint32_t block;
  uint8_t index;
} FilePtr;

/// Stores all arguments to complete a full card command, order matters!
typedef struct {
  uint8_t crc;
  uint32_t arg;
  uint8_t cmd;
} CardCommand;

/// Stores all information about a loaded volume
typedef struct {
  uint8_t isInitialized;     /* Indicates whether the volume is successfully initialized. Reset when errors occur */
  uint8_t isMultiFat;        /* More than one FAT on the volume */
  uint8_t isfat16;           /* volume type is FAT16, FAT32 otherwise */
  uint8_t blocksPerCluster;  /* cluster size in blocks */
  uint32_t blocksPerFat;     /* FAT size in blocks */
  uint32_t clusterLast;      /* the index of the first cluster that is out of range */
  uint32_t rootCluster;      /* First cluster of a FAT32 root directory, for FAT16: root offset */
  uint32_t rootSize;         /* Total size of a FAT16 root directory */
  uint32_t dataStartBlock;   /* first data block number */
  uint32_t fatStartBlock;    /* start block for first FAT */
} CardVolume;

/* ============================================================================== */

extern uint8_t card_notSDHCBlockShift;      /* Card is SD1 or SD2, and NOT SDHC. In that case this value is 9, 0 otherwise */

extern union SDMINFAT::cache_t volume_cacheBuffer;  /* 512 byte cache for device blocks */
extern uint32_t volume_cacheBlockNumber;   /* Logical number of block in the cache */
extern uint8_t  volume_cacheDirty;         /* readCache() will write current block first if true */
extern uint8_t  volume_cacheFATMirror;     /* current block in cache is a mirrored FAT block */

extern CardVolume volume;                  /* stores all current volume information */

extern uint8_t   file_isroot16dir;         /* file is a FAT16 root directory */
extern uint32_t  file_curCluster;          /* cluster for current file position */
extern uint32_t  file_position;            /* current file position in bytes from beginning */
extern FilePtr   file_curDir;              /* directory currently selected */
extern uint32_t  file_size;                /* total size of the currently opened file */

/* ============================================================================== */

/// Writes out a command to the card
uint8_t card_command(uint8_t cmd, uint32_t arg, uint8_t crc);
/// Waits to receive a given token from the card
uint8_t card_waitForData(uint8_t data_state);
/// Turns the card chip-select on or off
void card_setEnabled(uint8_t enabled);

/// Writes out the current cached block
void volume_writeCache(void);
/// Writes out the current cached block to the block specified
void volume_writeCache(uint32_t block);
/// Gets the first block of a cluster
inline uint32_t volume_firstClusterBlock(uint32_t cluster) {
  return volume.dataStartBlock + ((cluster - 2) * volume.blocksPerCluster);
}
/// Caches the current block to be read/written
uint8_t* volume_cacheCurrentBlock(uint8_t writeCluster);
/// Reads in the cache at the block specified
void volume_readCache(uint32_t blockNumber);
/// Gets the next cluster in the FAT cluster chain
uint8_t volume_fatGet(uint32_t cluster, uint32_t* value);
/// Puts the next cluster in the FAT cluster chain
void volume_fatPut(uint32_t cluster, uint32_t value);
/// Initializes the volume
uint8_t volume_init(uint8_t resetPosition = 1);

/**
 * @brief Initializes the card, then attempts to find and open the file
 * 
 * The filename must be 8 characters long, padded with spaces at the end if needed.
 * The extension must be 3 characters long, padded with spaces at the end if needed.
 * The mode specified the way to open the file, where SDMIN_FILE_READ, SDMIN_FILE_WRITE
 * and SDMIN_FILE_CREATE are possible
 */
uint8_t file_open(const char* filename, const char* ext, uint8_t mode);
/// Deletes all contents of a file (must call after opening file)
void file_truncate();
/// Flushes any pending block writes to the card
void file_flush(void);
/// Reads the current directory entry information into the cache
SDMINFAT::dir_t* file_readCacheDir(void);
/// Reads a single full HEX line (intel HEX format)
uint8_t file_read_hex_line(uint8_t* buff);
/// Writes a single full HEX line (intel HEX format)
void file_append_hex_line(uint8_t* buff, uint8_t len, uint32_t address, unsigned char recordType);
/// Reads a next block of data, size being fixed increments of 1/2/4/8/16/32/64/128/256/512
char* file_read(uint16_t nByteIncrement);
/// Writes a new block of data, size being fixed increments of 1/2/4/8/16/32/64/128/256/512
void file_write(const char* data, uint16_t nBytes);
/// Reads a single byte
char file_read_byte(void);
/// Writes a single byte
void file_write_byte(char b);
/// Writes a single byte at the end of the file
void file_append_byte(char b);

/// Deletes the currently opened file
void file_delete(void);
/// Saves the currently opened file under a new name, extension is preserved
void file_save(char filename[8]);

/* ============================================================================== */

#endif /* SDMINIMAL_H_ */