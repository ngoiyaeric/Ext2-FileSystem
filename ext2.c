#include "ext2.h"

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fsuid.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#define EXT2_OFFSET_SUPERBLOCK 1024

/* open_volume_file: Opens the specified file and reads the initial
   EXT2 data contained in the file, including the boot sector, file
   allocation table and root directory.

   Parameters:
     filename: Name of the file containing the volume data.
   Returns:
     A pointer to a newly allocated volume_t data structure with
     all fields initialized according to the data in the volume file,
     or NULL if the file is invalid or data is missing.
 */
volume_t *open_volume_file(const char *filename)
{

  int fd = open(filename, O_RDONLY);
  if (fd == -1)
    return NULL;

  struct stat vol_st;
  if (fstat(fd, &vol_st) == -1)
  {
    close(fd);
    return NULL;
  }

  volume_t *volume = malloc(sizeof(volume_t));
  volume->fd = fd;
  volume->volume_size = vol_st.st_size;

  /* TO BE COMPLETED BY THE STUDENT */
  ssize_t x = pread(fd, &volume->super, sizeof(superblock_t), EXT2_OFFSET_SUPERBLOCK);

  if (x != sizeof(superblock_t))
    return NULL;
  if (volume->super.s_magic != EXT2_SUPER_MAGIC)
    return NULL;

  volume->block_size = 1024 << volume->super.s_log_block_size;
  
  int offset;
  if (volume->block_size == 1024)
  {
    offset = 2048;
  }
  else
  {
    offset = volume->block_size;
  }


  volume->num_groups = (volume->super.s_blocks_count - 1) / volume->super.s_blocks_per_group + 1;

  volume->groups = malloc(sizeof(group_desc_t) * volume->num_groups);
  pread(fd, volume->groups, volume->num_groups*sizeof(group_desc_t), offset);

  return volume;
}

/* close_volume_file: Frees and closes all resources used by a EXT2 volume.

   Parameters:
     volume: pointer to volume to be freed.
 */
void close_volume_file(volume_t *volume)
{

  close(volume->fd);
  free(volume->groups);
  free(volume);
}

/* read_block: Reads data from one or more blocks. Saves the resulting
   data in buffer 'buffer'. This function also supports sparse data,
   where a block number equal to 0 sets the value of the corresponding
   buffer to all zeros without reading a block from the volume.

   Parameters:
     volume: pointer to volume.
     block_no: Block number where start of data is located.
     offset: Offset from beginning of the block to start reading
             from. May be larger than a block size.
     size: Number of bytes to read. May be larger than a block size.
     buffer: Pointer to location where data is to be stored.

   Returns:
     In case of success, returns the number of bytes read from the
     disk. In case of error, returns -1.
 */
ssize_t read_block(volume_t *volume, uint32_t block_no, uint32_t offset, uint32_t size, void *buffer)
{

  /* TO BE COMPLETED BY THE STUDENT */
  if (block_no == EXT2_INVALID_BLOCK_NUMBER)
    return -1;

  if (block_no == 0)
  {
    memset(buffer, 0, size);
    return size;
  }

  ssize_t bytes = pread(volume->fd, buffer, size, offset + block_no*volume->block_size);
  // if (bytes > volume->block_size)
  //   return volume->block_size;

  return bytes;
}
