#pragma once

#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>

typedef struct superblock {
  uint32_t s_inodes_count;      // Total number of inodes
  uint32_t s_blocks_count;      // Total number of blocks
  uint32_t s_r_blocks_count;    // Number of reserved blocks
  uint32_t s_free_blocks_count; // Number of free blocks (including reserved)
  uint32_t s_free_inodes_count; // Number of free inodes
  uint32_t s_first_data_block;  // First data block (block containing superblock)
  uint32_t s_log_block_size;    // Block size = 1024 << s_log_block_size
  int32_t  s_log_frag_size;     // Fragment size = 1024 << s_log_frag_size
  uint32_t s_blocks_per_group;  // Total blocks per block group
  uint32_t s_frags_per_group;   // Fragments per block group
  uint32_t s_inodes_per_group;  // Total inodes per block group
  uint32_t s_mtime;             // Last mount time
  uint32_t s_wtime;             // Last write-access time
  uint16_t s_mnt_count;         // Mount count since last verified
  uint16_t s_max_mnt_count;     // Maximum mounts before a full check
  uint16_t s_magic;             // Must be equal to EXT2_SUPER_MAGIC
  uint16_t s_state;             // FS was unmounted cleanly (see above)
  uint16_t s_errors;            // Action if errors detected (see above)
  uint16_t s_minor_rev_level;   // Minor version level
  uint32_t s_lastcheck;         // Last file system check time
  uint32_t s_checkinterval;     // Maximum time between file system checks
  uint32_t s_creator_os;        // OS that created file system (see above)
  uint32_t s_rev_level;         // Major version level
  uint16_t s_def_resuid;        // User ID for reserved blocks (eg root)
  uint16_t s_def_resgid;        // Group ID for reserved blocks (eg root)

  uint32_t s_first_ino;         // First inode for standard files
  uint16_t s_inode_size;        // Inode size
  uint16_t s_block_group_nr;    // Block group number (for backups in groups)
  uint32_t s_feature_compat;    // Compatible features (may be ignored)
  uint32_t s_feature_incompat;  // Required features for mounting
  uint32_t s_feature_ro_compat; // Required features for writing
  uint8_t  s_uuid[16];          // UUID (unique ID)
  char     s_volume_name[16];   // Volume name
  char     s_last_mounted[64];  // Path where FS was last mounted
  uint32_t s_algo_bitmap;       // Compression algorithm support

  // Not included: performance hints, journaling support, dir index
  // support, mount options, reserved
} superblock_t;

typedef struct group_desc {
  uint32_t bg_block_bitmap;      // Block number of block containing block bitmap
  uint32_t bg_inode_bitmap;      // Block number of block containing inode bitmap
  uint32_t bg_inode_table;       // Starting block number for inode table
  uint16_t bg_free_blocks_count; // Number of free blocks in group
  uint16_t bg_free_inodes_count; // Number of free inodes in group
  uint16_t bg_used_dirs_count;   // Number of inodes allocated to directories
  uint16_t bg_pad;               // Padding
  char     bg_reserved[12];      // Reserved for future use
} group_desc_t;

typedef struct ext2volume {
  
  int fd;
  
  superblock_t super;

  // Values obtained from other fields, saved here for easier computation
  uint32_t block_size;
  uint32_t volume_size;

  uint32_t num_groups;
  group_desc_t *groups;
} volume_t;

typedef struct inode {
  uint16_t i_mode;        // Mode (type of file and permissions)
  uint16_t i_uid;         // Owner's user ID
  uint32_t i_size;        // File size (least significant 32 bits)
  uint32_t i_atime;       // Last access time
  uint32_t i_ctime;       // Creation time
  uint32_t i_mtime;       // Last modification time
  uint32_t i_dtime;       // Deletion time
  uint16_t i_gid;         // Owner's group ID
  uint16_t i_links_count; // Reference counter (number of hard links)
  uint32_t i_blocks;      // Number of 512-byte blocks reserved for this inode
  uint32_t i_flags;       // Flags
  uint32_t i_osd1;        // OS-dependant value
  union {
    struct {
      uint32_t i_block[12];   // Direct block numbers
      uint32_t i_block_1ind;  // 1-indirect block number
      uint32_t i_block_2ind;  // 2-indirect block number
      uint32_t i_block_3ind;  // 3-indirect block number
    };
    char i_symlink_target[60]; // Symbolic link target, if shorter than 60 bytes
  };
  uint32_t i_generation;  // File version (used for NFS)
  uint32_t i_file_acl;    // Block number for extended attributes
  uint32_t i_dir_acl;     // File size (most significant 32 bits)
  uint32_t i_faddr;       // Location of file fragment (deprecated)
  union {
    uint8_t i_osd2[12]; // OS-dependant value
    struct {
      uint8_t  l_i_frag;      // Linux only: fragment number (deprecated)
      uint8_t  l_i_fsize;     // Linux only: fragment size (deprecated)
      uint16_t l_i_reserved;  // Reserved for future use
      uint16_t l_i_uid_high;  // Linux only: high 16 bits of owner's user ID
      uint16_t l_i_gid_high;  // Linux only: high 16 bits of owner's group ID
      uint32_t l_i_reserved2; // Reserved for future use
    };
  };
} inode_t;

typedef struct dir_entry {
  uint32_t de_inode_no;  // inode number
  uint16_t de_rec_len;   // displacement to find next entry
  uint8_t  de_name_len;  // string length of the file name field
  uint8_t  de_file_type; // file type of file (not used in rev #0)
  char     de_name[256]; // name string
} dir_entry_t;

// Value for s_magic
#define EXT2_SUPER_MAGIC 0xEF53

// Values for s_state
#define EXT2_VALID_FS 1 // Unmounted cleanly
#define EXT2_ERROR_FS 2 // Errors detected

// Values for s_errors
#define EXT2_ERRORS_CONTINUE 1 // Ignore errors
#define EXT2_ERRORS_RO       2 // Mount as read-only
#define EXT2_ERRORS_PANIC    3 // Cause kernel panic

// Values for s_creator_os
#define EXT2_OS_LINUX   0
#define EXT2_OS_HURD    1
#define EXT2_OS_MASIX   2
#define EXT2_OS_FREEBSD 3
#define EXT2_OS_LITES   4

// Values for s_feature_ro_compat
#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001 // Sparse Superblock
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE   0x0002 // Large file support, 64-bit file size
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR    0x0004 // Binary tree sorted directory files

// Reserved inode numbers
#define EXT2_BAD_INO         1 // Inode with bad blocks (e.g., corrupted)
#define EXT2_ROOT_INO        2 // Root directory
#define EXT2_ACL_IDX_INO     3 // ACL index (deprecated)
#define EXT2_ACL_DATA_INO    4 // ACL data (deprecated)
#define EXT2_BOOT_LOADER_INO 5 // Boot loader
#define EXT2_UNDEL_DIR_INO   6 // Undelete (trash) directory

// Inode flags (i_flags)
#define EXT2_SECRM_FL        0x00000001     // must be overwritten before deletion
#define EXT2_UNRM_FL         0x00000002     // on deletion copy to temp
#define EXT2_COMPR_FL        0x00000004     // compressed file
#define EXT2_SYNC_FL         0x00000008     // must be synchronized with memory
#define EXT2_IMMUTABLE_FL    0x00000010     // must not change blocks
#define EXT2_APPEND_FL       0x00000020     // only append allowed, no overwriting
#define EXT2_NODUMP_FL       0x00000040     // do not delete if refcount is zero
#define EXT2_NOATIME_FL      0x00000080     // do not update i_atime on access
#define EXT2_DIRTY_FL        0x00000100     // Dirty (modified)
#define EXT2_COMPRBLK_FL     0x00000200     // One or more compressed blocks
#define EXT2_NOCOMPR_FL      0x00000400     // Data should be provided without uncompression
#define EXT2_ECOMPR_FL       0x00000800     // Compression error detected
#define EXT2_BTREE_FL        0x00001000     // Directory with B-tree format
#define EXT2_INDEX_FL        0x00001000     // Directory with hash indexed format
#define EXT2_IMAGIC_FL       0x00002000     // AFS directory
#define EXT3_JOURNAL_DATA_FL 0x00004000     // journal file data
#define EXT2_RESERVED_FL     0x80000000     // reserved for ext2 library

#define EXT2_INVALID_BLOCK_NUMBER ((uint32_t) -1)

// For ext2.c
volume_t *open_volume_file(const char *filename);
void close_volume_file(volume_t *volume);

ssize_t read_block(volume_t *volume, uint32_t block_no, uint32_t offset, uint32_t size, void *buffer);

// For ext2file.c
ssize_t read_inode(volume_t *volume, uint32_t inode_no, inode_t *buffer);
uint32_t get_inode_block_no(volume_t *volume, inode_t *inode, uint64_t block_idx);
ssize_t read_file_block(volume_t *volume, inode_t *inode, uint64_t offset, uint64_t max_size, void *buffer);
ssize_t read_file_content(volume_t *volume, inode_t *inode, uint64_t offset, uint64_t max_size, void *buffer);

// For ext2dir.c
int64_t next_directory_entry(volume_t *volume, inode_t *dir_inode, off_t *offset, dir_entry_t *dir_entry);
int64_t find_file_in_directory(volume_t *volume, inode_t *inode, const char *name, dir_entry_t *buffer);
uint32_t find_file_from_path(volume_t *volume, const char *path, inode_t *dest_inode);

// For ext2symlink.c
int32_t read_symlink_target(volume_t *volume, inode_t *inode, char *buffer, size_t size);

static inline int inode_is_regular_file(inode_t *inode) {
  return (inode->i_mode & S_IFMT) == S_IFREG;
}

static inline int inode_is_directory(inode_t *inode) {
  return (inode->i_mode & S_IFMT) == S_IFDIR;
}

static inline int inode_is_symlink(inode_t *inode) {
  return (inode->i_mode & S_IFMT) == S_IFLNK;
}

static inline uint64_t inode_file_size(volume_t *volume, inode_t *inode) {
  // If file system supports large file sizes and file is a regular file
  if ((volume->super.s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_LARGE_FILE) &&
      inode_is_regular_file(inode))
    return ((uint64_t) inode->i_dir_acl << 32) | inode->i_size;
  else
    return inode->i_size;
}
