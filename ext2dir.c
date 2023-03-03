#include "ext2.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* next_directory_entry: Reads and returns one entry in a
   directory. Can be called repeatedly for the same inode to obtain
   additional entries.

   Parameters:
     volume: Pointer to volume.
     dir_inode: Pointer to inode structure for the directory.
     offset: Pointer to a variable keeping track of the inode
             offset. This variable must be set to zero before the
             first call to this function. The variable will be updated
             with each subsequent call, so that calling this function
             repeatedly with the same offset location will return
             consecutive directory entries.
     dir_entry: Data structure where the resulting directory entry's
                data will be stored. The de_name data structure will
                contain the name as a null-terminated string.

   Returns:
     On success returns the inode number for the next entry. If the
     inode is not a directory, or there is an error reading the
     directory data, returns -1. If there are no more entries
     in the directory, returns 0 (zero).
 */
int64_t next_directory_entry(volume_t *volume, inode_t *dir_inode,
                             off_t *offset, dir_entry_t *dir_entry)
{

  /* TO BE COMPLETED BY THE STUDENT */
  if (*offset > dir_inode->i_size)
    return 0;
  if (!inode_is_directory(dir_inode))
    return -1;
  ssize_t bytes = read_file_content(volume, dir_inode, *offset, sizeof(dir_entry_t), dir_entry);
  *offset = *offset + dir_entry->de_rec_len;
  dir_entry->de_name[dir_entry->de_name_len] = '\0';
  if (*offset > dir_inode->i_size)
    return 0;

  while (dir_entry->de_inode_no == 0)
  {
    bytes = read_file_content(volume, dir_inode, *offset, sizeof(dir_entry_t), dir_entry);
    *offset = *offset + dir_entry->de_rec_len;
    dir_entry->de_name[dir_entry->de_name_len] = '\0';
    if (*offset > dir_inode->i_size)
      return 0;
    else if (bytes < 0)
      return -1;
  }

  if (bytes < 0)
  {
    return -1;
  }
  else
  {
    return dir_entry->de_inode_no;
  }
}

/* find_file_in_directory: Searches for a file in a directory.

   Parameters:
     volume: Pointer to volume.
     inode: Pointer to inode structure for the directory.
     name: NULL-terminated string for the name of the file. The file
           name must match this name exactly, including case.
     buffer: If the file is found, and this pointer is set to a
             non-NULL value, this buffer is set to the directory entry
             of the file. If the pointer is NULL, nothing is saved. If
             the file is not found, the value of this buffer is
             unmodified.

   Returns:
     If the file exists in the directory, returns the inode number
     associated to the file. If the inode is not a directory, or there
     is an error reading the directory data, returns -1. If the name
     does not exist, returns 0 (zero).
 */
int64_t find_file_in_directory(volume_t *volume, inode_t *inode, const char *name, dir_entry_t *buffer)
{

  /* TO BE COMPLETED BY THE STUDENT */
  off_t offset = 0;
  int64_t inode_no;
  if (inode_is_directory(inode) == 0) return -1;
  dir_entry_t *de = malloc(sizeof(dir_entry_t));


  while ((inode_no = next_directory_entry(volume, inode, &offset, de)) > 0)
  {
    if (strcmp(name, de->de_name) == 0)
    {
      if (buffer != NULL)
      {
        memcpy(buffer, de, sizeof(dir_entry_t));
      }
      inode_no = de->de_inode_no;
      free(de);
      return inode_no;
    }
  }
  return 0;
}

/* find_file_from_path: Searches for a file based on its full path.

   Parameters:
     volume: Pointer to volume.
     path: NULL-terminated string for the full absolute path of the
           file. Must start with '/' character. Path components
           (subdirectories) must be delimited by '/'. The root
           directory can be obtained with the string "/".
     dest_inode: If the file is found, and this pointer is set to a
                 non-NULL value, this buffer is set to the inode of
                 the file. If the pointer is NULL, nothing is
                 saved. If the file is not found, the value of this
                 buffer is unmodified.

   Returns:
     If the file exists, returns the inode number associated to the
     file. If the file does not exist, or there is an error reading
     any directory or inode in the path, returns 0 (zero).
 */
uint32_t find_file_from_path(volume_t *volume, const char *path, inode_t *dest_inode)
{

  /* TO BE COMPLETED BY THE STUDENT */
  dir_entry_t entry;
  inode_t inode;
  int64_t inode_no;
  char *p = strdup(path);
  char *fun;

  if (read_inode(volume, EXT2_ROOT_INO, &inode) < 0)
  {
    return 0;
  }

  fun = strtok(p, "/");

  while (fun != NULL)
  {
    while ((inode_no = find_file_in_directory(volume, &inode, fun, &entry)) > 0)
    {
      if (strcmp(entry.de_name, fun) == 0)
      {
        // strcat
        break;
      }
    }
  }

  // find_file_in_directory(volume, dest_inode, path, &entry);
  return entry.de_inode_no;
}
