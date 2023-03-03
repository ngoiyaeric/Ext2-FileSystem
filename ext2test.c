#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "ext2.h"

static void print_inode_blocks(volume_t *volume, inode_t *inode) {
  int num_blocks = (inode_file_size(volume, inode) - 1) / volume->block_size + 1;
  if (num_blocks > 50) num_blocks = 50;
  char *prefix = "";
  for (int i = 0; i < num_blocks; i++) {
    uint32_t block = get_inode_block_no(volume, inode, i);
    if (block) {
      printf("%s %i:%" PRIu32, prefix, i, block);
      prefix = ";";
    }
  }
  if (!*prefix) printf(" NONE");
}

static void print_inode_metadata(volume_t *volume, uint32_t inode_no, inode_t *inode) {
  printf("  Inode number : %#" PRIx32 "\n", inode_no);
  printf("  Mode         : %#" PRIo32 "\n", inode->i_mode);
  printf("  Size         : %" PRIu64 "\n", inode_file_size(volume, inode));
  printf("  Blocks       : %" PRIu32 "\n", inode->i_blocks);
  printf("  Block numbers:"); print_inode_blocks(volume, inode); printf("\n"); 
}

static void print_dir_entries(volume_t *volume, inode_t *inode) {
  
  off_t offset = 0;
  dir_entry_t entry;
  int64_t inode_no;
  printf("  Entries:\n");
  while ((inode_no = next_directory_entry(volume, inode, &offset, &entry)) > 0)
    printf("    %#8" PRIx32 ": %s\n", entry.de_inode_no, entry.de_name);
}

static void print_dir_entries_recursive(volume_t *volume, const char *name,
                                        uint32_t dir_inode_no, unsigned int recursion_level) {
  
  off_t offset = 0;
  dir_entry_t entry;
  int64_t child_inode_no;
  inode_t inode;

  if (!strcmp(".", name) || !strcmp("..", name)) return;
  
  ssize_t read_rv = read_inode(volume, dir_inode_no, &inode);
  assert(read_rv > 0);

  printf("%*s/%s (blocks:", recursion_level * 2, "", name);
  print_inode_blocks(volume, &inode);
  printf(")\n");

  while ((child_inode_no = next_directory_entry(volume, &inode, &offset, &entry)) > 0)
    print_dir_entries_recursive(volume, entry.de_name, entry.de_inode_no, recursion_level + 1);
}

int main(int argc, char *argv[]) {
  
  volume_t *volume;
  
  if (argc != 2) {
    fprintf(stderr, "Usage: %s volume_file\n", argv[0]);
    return 1;
  }

  errno = 0;
  volume = open_volume_file(argv[1]);
  if (!volume) {
    fprintf(stderr, "Provided volume file is invalid or incomplete: %s.\n", argv[1]);
    if (errno != 0)
      fprintf(stderr, "\t%s\n", strerror(errno));
    return 1;
  }

  printf("Volume name           : %s\n", volume->super.s_volume_name);
  printf("Last mounted on       : %s\n", volume->super.s_last_mounted);
  printf("File system version   : %" PRIu32 ".%" PRIu16 "\n",
	 volume->super.s_rev_level, volume->super.s_minor_rev_level);
  printf("Status                : %" PRIu16 " - %s\n\n", volume->super.s_state,
	 volume->super.s_state == EXT2_VALID_FS ? "Unmounted cleanly" : "Errors detected");
  
  printf("Total size (in bytes) : %" PRIu32 "\n", volume->volume_size);
  printf("Block size (in bytes) : %" PRIu32 "\n", volume->block_size);
  printf("Total number of blocks: %" PRIu32 "\n", volume->super.s_blocks_count);
  printf("Total number of inodes: %" PRIu32 "\n", volume->super.s_inodes_count);
  printf("No of reserved blocks : %" PRIu32 "\n", volume->super.s_r_blocks_count);
  printf("Blocks per group      : %" PRIu32 "\n", volume->super.s_blocks_per_group);
  printf("Inodes per group      : %" PRIu32 "\n", volume->super.s_inodes_per_group);

  for (int g = 0; g < volume->num_groups; g++) {
    
    printf("\n== BLOCK GROUP %d ==\n", g);
    printf("Number of free blocks : %" PRIu32 "\n", volume->groups[g].bg_free_blocks_count);
    printf("Number of free inodes : %" PRIu32 "\n", volume->groups[g].bg_free_inodes_count);
    printf("No of directory inodes: %" PRIu32 "\n", volume->groups[g].bg_used_dirs_count);
  }

  uint32_t inode_no;
  inode_t inode;
  dir_entry_t entry;
  char content[128] = "<BUFFER NOT INITIALIZED>";
  
  printf("\n10 bytes at offset 17 of block 534:\n");
  if (read_block(volume, 534, 17, 10, content) < 0) {
    printf("  NOT FOUND!!!\n");
  } else {
    printf("  >>>%.10s<<<\n", content);
  }
  
  printf("\nRoot directory (via inode number):\n");
  inode_no = EXT2_ROOT_INO;
  if (read_inode(volume, inode_no, &inode) < 0) {
    printf("  NOT FOUND!!!\n");
  } else {
    print_inode_metadata(volume, inode_no, &inode);
    print_dir_entries(volume, &inode);
  }
  
  printf("\nDirectory d1 (from root inode):\n");
  if (!(inode_no = find_file_in_directory(volume, &inode, "d1", &entry))) {
    printf("  NOT FOUND (FILE)!!!\n");
  } else if (read_inode(volume, entry.de_inode_no, &inode) < 0) {
    printf("  NOT FOUND (INODE)!!!\n");
  } else {
    print_inode_metadata(volume, inode_no, &inode);
    print_dir_entries(volume, &inode);
  }
  
  printf("\nRoot directory (via path):\n");
  if (!(inode_no = find_file_from_path(volume, "/", &inode))) {
    printf("  NOT FOUND!!!\n");
  } else {
    print_inode_metadata(volume, inode_no, &inode);
    print_dir_entries(volume, &inode);
  }
  
  printf("\nDirectory d1 (via path):\n");
  if (!(inode_no = find_file_from_path(volume, "/d1", &inode))) {
    printf("  NOT FOUND!!!\n");
  } else {
    print_inode_metadata(volume, inode_no, &inode);
    print_dir_entries(volume, &inode);
  }
  
  printf("\nDirectory d1/d2:\n");
  if (!(inode_no = find_file_from_path(volume, "/d1/d2", &inode))) {
    printf("  NOT FOUND!!!\n");
  } else {
    print_inode_metadata(volume, inode_no, &inode);
    print_dir_entries(volume, &inode);
  }
  
  printf("\nDirectory d1/d2/d3/d4/d5:\n");
  if (!(inode_no = find_file_from_path(volume, "/d1/d2/d3/d4/d5", &inode))) {
    printf("  NOT FOUND!!!\n");
  } else {
    print_inode_metadata(volume, inode_no, &inode);
    print_dir_entries(volume, &inode);
  }
  
  printf("\nFile termcap:\n");
  if (!(inode_no = find_file_from_path(volume, "/termcap", &inode))) {
    printf("  NOT FOUND!!!\n");
  } else {
    print_inode_metadata(volume, inode_no, &inode);
  }
  
  printf("\nFile d1/File1.txt:\n");
  if (!(inode_no = find_file_from_path(volume, "/d1/File1.txt", &inode))) {
    printf("  NOT FOUND!!!\n");
  } else {
    print_inode_metadata(volume, inode_no, &inode);
    printf("  Content      : ");
    int rv = read_file_content(volume, &inode, 0, sizeof(content), content);
    printf("%.*s\n", rv, content);
  }
  
  printf("\nFile d1/d2/sparse/Bigfile2.txt:\n");
  if (!(inode_no = find_file_from_path(volume, "/d1/d2/sparse/Bigfile2.txt", &inode))) {
    printf("  NOT FOUND!!!\n");
  } else {
    print_inode_metadata(volume, inode_no, &inode);
  }
  
  printf("\nSymlink ImageInst.txt:\n");
  if (!(inode_no = find_file_from_path(volume, "/ImageInst.txt", &inode))) {
    printf("  NOT FOUND!!!\n");
  } else {
    print_inode_metadata(volume, inode_no, &inode);

    if (read_symlink_target(volume, &inode, content, sizeof(content)))
      printf("  Target       : %s\n", content);
    else
      printf("  Could not read target!!!\n");
  }

  printf("\nFull list of files:\n");
  print_dir_entries_recursive(volume, "", EXT2_ROOT_INO, 0);
  
  return 0;
}
