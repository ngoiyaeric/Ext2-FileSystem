#include "ext2.h"

/* read_inode: Fills an inode data structure with the data from one
   inode in disk. Determines the block group number and index within
   the group from the inode number, then reads the inode from the
   inode table in the corresponding group. Saves the inode data in
   buffer 'buffer'.

   Parameters:
     volume: pointer to volume.
     inode_no: Number of the inode to read from disk.
     buffer: Pointer to location where data is to be stored.

   Returns:
     In case of success, returns a positive value. In case of error,
     returns -1.
 */
ssize_t read_inode(volume_t *volume, uint32_t inode_no, inode_t *buffer)
{

  /* TO BE COMPLETED BY THE STUDENT */
  // int inode_table = volume->groups->bg_inode_table;

  if (inode_no == 0 || inode_no > volume->super.s_inodes_count)
    return -1;

  int inumber = inode_no - 1;
  int group_no = inumber / volume->super.s_inodes_per_group;
  int containing_block = volume->groups[group_no].bg_inode_table;

  int inode_index = inumber % volume->super.s_inodes_per_group;

  int offset = containing_block * volume->block_size + inode_index * sizeof(inode_t);

  return pread(volume->fd, buffer, sizeof(inode_t), offset);
}

/* get_inode_block_no: Returns the block number containing the data
   associated to a particular index. For indices 0-11, returns the
   direct block number; for larger indices, returns the block number
   at the corresponding indirect block.

   Parameters:
     volume: Pointer to volume.
     inode: Pointer to inode structure where data is to be sourced.
     index: Index to the block number to be searched.

   Returns:
     In case of success, returns the block number to be used for the
     corresponding entry. This block number may be 0 (zero) in case of
     sparse files. In case of error, returns
     EXT2_INVALID_BLOCK_NUMBER.
 */
uint32_t get_inode_block_no(volume_t *volume, inode_t *inode, uint64_t block_idx)
{

  /* TO BE COMPLETED BY THE STUDENT */

  if (block_idx >= 0 && block_idx < 12)
    return inode->i_block[block_idx];

  int block_no_count = volume->block_size / 4;

  block_idx = block_idx - 12;

  if (block_idx < block_no_count) // 1-indirect block
  {
    uint32_t block_no = 0;
    ssize_t b = read_block(volume, inode->i_block_1ind, block_idx * 4, 4, &block_no);
    if (b == -1)
      return EXT2_INVALID_BLOCK_NUMBER;
    return block_no;
  }
  else if (block_idx < (block_no_count * block_no_count + block_no_count)) // 2-indirect block
  {
    // inode->i_block_2ind stores start of 2-indirect table
    // we need the index of this table - we get it by (block_idx - block_no_count) / block_no_count
    block_idx = block_idx - block_no_count;
    int first_index = block_idx / block_no_count;

    // now we need to read block starting at inode->i_block_2ind with offset first_index
    // this will return a block number, which is an index to a 1-indirect table

    uint32_t second_index;
    ssize_t b = read_block(volume, inode->i_block_2ind, first_index * 4, 4, &second_index);
    if (b == -1)
      return EXT2_INVALID_BLOCK_NUMBER;
    // now we have the index to the start of the 1-indirect table
    // however, we still need the offset to this table
    // we derive this offset by: (block_idx - block_no_count) % block_no_count
    // then we call read_block and store its result in block_no

    uint32_t block_no = 0;
    uint32_t offset = block_idx % block_no_count;
    b = read_block(volume, second_index, offset * 4, 4, &block_no);
    if (b == -1)
      return EXT2_INVALID_BLOCK_NUMBER;
    return block_no;
  }
  else if (block_idx < (block_no_count * block_no_count * block_no_count + block_no_count * block_no_count + block_no_count)) // 3-indirect block
  {
    // inode->i_block_3ind stores start of 3-indirect table
    // we need the index of this table - we get it by (block_idx - block_no_count - block_no_count^2) / block_no_count^2

    block_idx = block_idx - block_no_count - (block_no_count * block_no_count);
    int first_index = block_idx / (block_no_count * block_no_count);

    // now we need to read block starting at inode->i_block_3ind with offset first_index
    // this will return a block number, which is an index to a 2-indirect table

    int second_index;
    ssize_t b = read_block(volume, inode->i_block_3ind, first_index * 4, 4, &second_index);
    if (b == -1)
      return EXT2_INVALID_BLOCK_NUMBER;

    // now we have the index to the start of the 2-indirect table
    // however, we still need the offset to this table
    // we derive this offset by: (block_idx - block_no_count - block_no_count^2) % block_no_count^2
    // then we call read_block and store its result in third_index

    uint32_t offset = block_idx % (block_no_count * block_no_count);
    // offset = offset / block_no_count;

    uint32_t third_index;
    b = read_block(volume, second_index, (offset / block_no_count) * 4, 4, &third_index);
    if (b == -1)
      return EXT2_INVALID_BLOCK_NUMBER;

    // uint32_t final_offset = offset % block_no_count;
    uint32_t block_no = 0;
    b = read_block(volume, third_index, (offset % block_no_count) * 4, 4, &block_no);
    if (b == -1)
      return EXT2_INVALID_BLOCK_NUMBER;

    return block_no;
  }
  else
  {
    return EXT2_INVALID_BLOCK_NUMBER;
  }
}

/* read_file_block: Returns the content of a specific file, limited to
   a single block.

   Parameters:
     volume: Pointer to volume.
     inode: Pointer to inode structure for the file.
     offset: Offset, in bytes from the start of the file, of the data
             to be read.
     max_size: Maximum number of bytes to read from the block.
     buffer: Pointer to location where data is to be stored.

   Returns:
     In case of success, returns the number of bytes read from the
     disk. In case of error, returns -1.
 */
ssize_t read_file_block(volume_t *volume, inode_t *inode, uint64_t offset, uint64_t max_size, void *buffer)
{

  /* TO BE COMPLETED BY THE STUDENT */
  // uint32_t read_so_far = 0;

  // int size = max_size;
  if (offset + max_size > inode_file_size(volume, inode)){
    max_size = inode_file_size(volume, inode) - offset;
  }
  if ((offset%volume->block_size) + max_size > volume->block_size){
    max_size = volume->block_size - (offset%volume->block_size);
  }

  ssize_t rv = read_block(volume, get_inode_block_no(volume, inode, offset / volume->block_size), (offset % volume->block_size), max_size, buffer);

  return rv;
}

/* read_file_content: Returns the content of a specific file, limited
   to the size of the file only. May need to read more than one block,
   with data not necessarily stored in contiguous blocks.

   Parameters:
     volume: Pointer to volume.
     inode: Pointer to inode structure for the file.
     offset: Offset, in bytes from the start of the file, of the data
             to be read.
     max_size: Maximum number of bytes to read from the file.
     buffer: Pointer to location where data is to be stored.

   Returns:
     In case of success, returns the number of bytes read from the
     disk. In case of error, returns -1.
 */
ssize_t read_file_content(volume_t *volume, inode_t *inode, uint64_t offset, uint64_t max_size, void *buffer)
{

  uint32_t read_so_far = 0;

  if (offset + max_size > inode_file_size(volume, inode))
    max_size = inode_file_size(volume, inode) - offset;

  while (read_so_far < max_size)
  {
    int rv = read_file_block(volume, inode, offset + read_so_far,
                             max_size - read_so_far, buffer + read_so_far);
    if (rv <= 0)
      return rv;
    read_so_far += rv;
  }
  return read_so_far;
}
