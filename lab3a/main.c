//
//  main.c
//  lab3a
//
//  Created by Jingchi Ma on 5/21/19.
//  Copyright © 2019 Jingchi Ma. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "ext2_fs.h"

#define BLOCK_SIZE EXT2_MIN_BLOCK_SIZE

/* file types */
#define EXT2_S_IFREG 0x8000
#define EXT2_S_IFDIR 0x4000
#define EXT2_S_IFLNK 0xA000


const int arg_error = 1; // exit code when input argument is wrong
const int op_err = 2;    // all other errors
int fs_fd = -1; // file descriptor for the file system

typedef struct {
    __u32 n_blocks;
    __u32 n_inodes;
    __u32 block_size;
    __u16 inode_size;
    __u32 blocks_per_group;
    __u32 inodes_per_group;
    __u32 first_nr_inode;
    __u32 n_group;
} superblock_info;
typedef struct {
    __u32 n_blocks;
    __u32 n_inodes;
    __u16 n_free_block;
    __u16 n_free_inode;
    __u32 block_bitmap;
    __u32 inode_bitmap;
    __u32 first_inode;
    
} group_info;
superblock_info sb;
group_info *groups;

void err_exit(int exit_code, char* err_msg) {
    if (errno != 0) {
        perror(err_msg);
    } else {
        fprintf(stderr, "%s", err_msg);
    }
    exit(exit_code);
}

void superblock_summary() {
    struct ext2_super_block superblock_buffer;
    if (pread(fs_fd, &superblock_buffer, sizeof(superblock_buffer), BLOCK_SIZE) == -1) {
        err_exit(op_err, "not able to read superblock");
    }
    
    /* store superblock data */
    sb.n_blocks          = superblock_buffer.s_blocks_count;
    sb.n_inodes         = superblock_buffer.s_inodes_count;
    // bsize = EXT2_MIN_BLOCK_SIZE << s_log_block_size
    sb.block_size       = EXT2_MIN_BLOCK_SIZE << superblock_buffer.s_log_block_size;
    sb.inode_size       = superblock_buffer.s_inode_size;
    sb.blocks_per_group = superblock_buffer.s_blocks_per_group;
    sb.inodes_per_group = superblock_buffer.s_inodes_per_group;
    sb.first_nr_inode   = superblock_buffer.s_first_ino;
    
    /* printf csv to stdout */
    /* summary of report
     SUPERBLOCK
     total number of blocks (decimal)
     total number of i-nodes (decimal)
     block size (in bytes, decimal)
     i-node size (in bytes, decimal)
     blocks per group (decimal)
     i-nodes per group (decimal)
     first non-reserved i-node (decimal)
     */
    fprintf(stdout, "%s,%d,%d,%d,%d,%d,%d,%d\n",
            "SUPERBLOCK",
            sb.n_blocks,
            sb.n_inodes,
            sb.block_size,
            sb.inode_size,
            sb.blocks_per_group,
            sb.inodes_per_group,
            sb.first_nr_inode
            );
}

void group_summary() {
    /* First needs to get number of group. Should be one in this project */
    sb.n_group = sb.n_blocks / sb.blocks_per_group + 1;
    groups = malloc(sizeof(group_info) * sb.n_group);
    __u32 i;
    __u32 offset = BLOCK_SIZE * 2;
    group_info* cur_group = groups;
    for (i = 0; i < sb.n_group; i++) {
        /* for each block group */
        struct ext2_group_desc group_desc;
        if (pread(fs_fd, &group_desc, sizeof(group_desc), offset) == -1) {
            err_exit(2, "unable to read group descriptor blocks");
        }
        /* group summary
         GROUP
         group number (decimal, starting from zero)
         total number of blocks in this group (decimal)
         total number of i-nodes in this group (decimal)
         number of free blocks (decimal)
         number of free i-nodes (decimal)
         block number of free block bitmap for this group (decimal)
         block number of free i-node bitmap for this group (decimal)
         block number of first block of i-nodes in this group (decimal)
         */
        /* number of blocks and inodes  */
        if (i == sb.n_group - 1) { // last group
            cur_group->n_blocks = sb.n_blocks - i * sb.blocks_per_group;
            cur_group->n_inodes = sb.n_inodes - i * sb.inodes_per_group;
        } else {
            cur_group->n_blocks = sb.blocks_per_group;
            cur_group->n_inodes = sb.inodes_per_group;
        }
        /* number of free blocks and inodes */
        cur_group->n_free_block = group_desc.bg_free_blocks_count;
        cur_group->n_free_inode = group_desc.bg_free_inodes_count;
        
        /* block number of block and inode bitmap */
        cur_group->block_bitmap = group_desc.bg_block_bitmap;
        cur_group->inode_bitmap = group_desc.bg_inode_bitmap;
        cur_group->first_inode  = group_desc.bg_inode_table;
        
        fprintf(stdout, "%s,%d,%d,%d,%d,%d,%d,%d,%d\n",
                "GROUP",
                i,
                cur_group->n_blocks,
                cur_group->n_inodes,
                cur_group->n_free_block,
                cur_group->n_free_inode,
                cur_group->block_bitmap,
                cur_group->inode_bitmap,
                cur_group->first_inode
                );
        offset += sizeof(group_desc);
        cur_group++;
    }
    

}

void free_block_entries_summary() {
    __u32 i;
    for (i = 0; i < sb.n_group; i++) {
        __u32 bitmap_bn = groups[i].block_bitmap;
        char *bitmap = malloc(BLOCK_SIZE);
        if (pread(fs_fd, bitmap, BLOCK_SIZE, BLOCK_SIZE * bitmap_bn) == -1) {
            err_exit(2, "ERROR in free block entries: read block bitmap failed");
        }
        __u32 j;
        __u32 block_number = 1; // block number starts with 1
        /* be sure to check if block_number <= total number of blocks in this group */
        for (j = 0; j < BLOCK_SIZE && block_number <= groups[i].n_blocks; j++) {
            __u32 k;
            for (k = 0; k < 8 && block_number <= groups[i].n_blocks; k++, block_number++) {
                char cur = (bitmap[j] >> k) & 0x01;
                if (cur == 0) {
                    fprintf(stdout, "%s,%d\n", "BFREE", block_number);
                }
            }
        }
        free(bitmap);
    }
}

void free_inode_entries_summary() {
    __u32 i;
    for (i = 0; i < sb.n_group; i++) {
        __u32 bitmap_bn = groups[i].inode_bitmap;
        char *bitmap = malloc(BLOCK_SIZE);
        if (pread(fs_fd, bitmap, BLOCK_SIZE, BLOCK_SIZE * bitmap_bn) == -1) {
            err_exit(2, "ERROR in free_inode_entries_summary: read inode bitmap failed");
        }
        __u32 j;
        __u32 block_number = 1;
        for (j = 0; j < BLOCK_SIZE && block_number <= groups[i].n_blocks; j++) {
            __u32 k;
            for (k = 0; k < 8 && block_number <= groups[i].n_blocks; k++, block_number++) {
                char cur = (bitmap[j] >> k) & 0x01;
                if (cur == 0) {
                    fprintf(stdout, "%s,%d\n", "IFREE", block_number);
                }
            }
        }
    }
}


// --------------- helper functions for inode_summary -------------
char get_file_type(struct ext2_inode *inode) {
    switch(inode->i_mode & 0xF000) {
        case EXT2_S_IFREG:
            return 'f';
        case EXT2_S_IFDIR:
            return 'd';
        case EXT2_S_IFLNK:
            return 's';
        default:
            return '?';
    }
}
void get_gmt(time_t ts, char* time_string, __u32 size) {
    struct tm *gmt;
    gmt = gmtime(&ts);
    strftime(time_string, size, "%m/%d/%y %H:%M:%S", gmt);
}
// ----------------------------------------------------------------

void inode_summary() {
    __u32 group_idx;
    for (group_idx = 0; group_idx < sb.n_group; group_idx++) {
        __u32 first_inode_bn = groups[group_idx].first_inode;
        __u32 offset = first_inode_bn * BLOCK_SIZE;
        __u32 i; // i is the inode number
        for (i = 1; i <= groups[group_idx].n_inodes;
             i++, offset += sizeof(struct ext2_inode)) {

            struct ext2_inode cur_inode;
            if (pread(fs_fd, &cur_inode, sizeof(cur_inode), offset) == -1) {
                err_exit(2, "ERROR in inode_summary: read inode fails");
            }
            if (cur_inode.i_links_count == 0 || cur_inode.i_mode == 0) {
                continue;
            }
            char filetype = get_file_type(&cur_inode);
            
            char c_time[32];
            char m_time[32];
            char a_time[32];
            get_gmt(cur_inode.i_ctime, c_time, sizeof(c_time));
            get_gmt(cur_inode.i_mtime, m_time, sizeof(m_time));
            get_gmt(cur_inode.i_atime, a_time, sizeof(a_time));
            
            fprintf(stdout,"%s,%u,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",
                    "INODE",
                    i,
                    filetype,
                    cur_inode.i_mode & 0x0FFF,
                    cur_inode.i_uid,
                    cur_inode.i_gid,
                    cur_inode.i_links_count,
                    c_time,
                    m_time,
                    a_time,
                    cur_inode.i_size,
                    cur_inode.i_blocks
                    );
            if (filetype == '?' || filetype == 's') {
                fprintf(stdout, "\n");
            } else {
                /* regular file or directory */
                // don't forget the initial ","
                fprintf(stdout, ",%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
                        cur_inode.i_block[0],
                        cur_inode.i_block[1],
                        cur_inode.i_block[2],
                        cur_inode.i_block[3],
                        cur_inode.i_block[4],
                        cur_inode.i_block[5],
                        cur_inode.i_block[6],
                        cur_inode.i_block[7],
                        cur_inode.i_block[8],
                        cur_inode.i_block[9],
                        cur_inode.i_block[10],
                        cur_inode.i_block[11],
                        cur_inode.i_block[12],
                        cur_inode.i_block[13],
                        cur_inode.i_block[14]
                        );
            }
        }
    }
}

// ------- helper fields for indirect access ------------------
const __u32 N_BLOCK = BLOCK_SIZE / sizeof(__u32);
const __u32 indirect_offsets[] = {1, N_BLOCK, N_BLOCK * N_BLOCK};
// ------------------------------------------------------------


// ------- helper functions for directory_entries_summary ---------
/*
 @param block_num: the block number of current block
 @param logical_offset: logical offset of the current block
 @param parent_inode_num: inode number of inode which owns the data block
 The current block must be a data block.
 This function will scan all possible directory entries and print them out.
 */
void directory_entries_in_block(__u32 block_num, __u32 *logical_offset, __u32 parent_inode_num) {
    __u32 start = 0; // start of next directory entry
    __u32 base_offset = block_num * BLOCK_SIZE;
    while (start < BLOCK_SIZE) {
        // cannot have directory entry spanning data blocks, and if there's no enough space,
        // the directory entry will have to be stored on another block.
        // See http://www.nongnu.org/ext2-doc/ext2.html#linked-directories for more detail.
        struct ext2_dir_entry dir_entry;
        if (pread(fs_fd, &dir_entry, sizeof(dir_entry), base_offset + start) == -1) {
            err_exit(2, "ERROR in directory_entry_results: loading directory entry failed");
        }
        /* copy name to buffer to make sure it's null terminated */
        char name_buffer[dir_entry.name_len + 1];
        memcpy(name_buffer, dir_entry.name, dir_entry.name_len);
        name_buffer[dir_entry.name_len] = 0;
        if (dir_entry.inode != 0) {
            fprintf(stdout, "%s,%d,%u,%u,%u,%u,'%s'\n",
                    "DIRENT",
                    parent_inode_num,
                    *logical_offset,
                    dir_entry.inode,
                    dir_entry.rec_len,
                    dir_entry.name_len,
                    name_buffer
                    );
        } else {
            break; // no directory entry in this block
        }
        start += dir_entry.rec_len; // dir_entry.length is the distance to the next directory entry
        *logical_offset += dir_entry.rec_len;
    }
}

/*
 @param level: level of indirection.
 @param block_num: the block_num of current (pointer) block
 @param logical_offset: the offset (of the first data block accessible via the current block) w.r.t. the current file.
 @inode: the inode owning the block.
 This function will print results for all non-zero pointers recursively.
 */
void process_block_directory(__u32 level, __u32 block_num,
                             __u32 *logical_offset, __u32 inode_num) {
    // if level == 0, then this is data block, print out entry results
    // otherwise, update offset and level and continue recursion.
    if (level == 0) {
        /* data block */
        directory_entries_in_block(block_num, logical_offset, inode_num);
        return;
    }
    __u32 cur_addresses[N_BLOCK];
    if (pread(fs_fd, cur_addresses, BLOCK_SIZE, block_num * BLOCK_SIZE) == -1) {
        err_exit(2, "ERROR in process_indirect_block: loading current block failed");
    }
    __u32 addr_idx = 0;
    for (; addr_idx < N_BLOCK; addr_idx++) {
        __u32 next_level_bn = cur_addresses[addr_idx];
        if (next_level_bn == 0) {
            /* not pointing to a valid block */
            continue;
        }
        process_block_directory(level - 1, next_level_bn,
                                logical_offset,
                                inode_num);
    }
}
// ----------------------------------------------------------------

void directory_entries_summary() {
    __u32 group_idx;
    for (group_idx = 0; group_idx < sb.n_group; group_idx++) {
        __u32 first_inode_bn = groups[group_idx].first_inode;
        __u32 offset = first_inode_bn * BLOCK_SIZE;
        __u32 inodenum = 1;
        for (; inodenum <= groups[group_idx].n_inodes;
             inodenum++, offset += sizeof(struct ext2_inode)) {
            struct ext2_inode cur_inode;
            if (pread(fs_fd, &cur_inode, sizeof(struct ext2_inode), offset) == -1) {
                err_exit(2, "ERROR in directory_entries: read inode failed");
            }
            if (cur_inode.i_mode & EXT2_S_IFDIR) {
                /* is directory */
                __u32 k; // index to scan the referencing blocks
                __u32 logical_offset = 0;
                __u32 level = 0;
                for (k = 0; k < EXT2_N_BLOCKS; k++) {
                    if (cur_inode.i_block[k] == 0) {
                        continue; // no block
                    }
                    if (k < EXT2_NDIR_BLOCKS) {
                        /* direct blocks */
                        level = 0;
                    } else if (k == EXT2_IND_BLOCK) {
                        /* single indirect block */
                        level = 1;
                    } else if (k == EXT2_DIND_BLOCK) {
                        /* double indirect block */
                        level = 2;
                    } else {
                        /* triple indirect block */
                        level = 3;
                    }
                    process_block_directory(level, cur_inode.i_block[k],
                                            &logical_offset, inodenum);
                }
            }
        }
    }
}

// ------------ helper methods for indirect_block_references_summary -----------
/*
 @param level: level of indirection.
 @param block_num: the block_num of current (pointer) block
 @param logical_offset: the offset (of the first data block accessible via the current block) w.r.t. the current file.
 @inode: the inode owning the block.
 This function will print results for all non-zero pointers recursively.
 */
void process_indirect_block(__u32 level, __u32 block_num, __u32 logical_offset, __u32 inode_num) {
    __u32 cur_addresses[N_BLOCK];
    if (pread(fs_fd, cur_addresses, BLOCK_SIZE, block_num * BLOCK_SIZE) == -1) {
        err_exit(2, "ERROR in process_indirect_block: loading current block failed");
    }
    __u32 address_idx;
    for (address_idx = 0; address_idx < N_BLOCK; address_idx++) {
        __u32 next_level_bn = cur_addresses[address_idx];
        if (next_level_bn == 0) {
            // block number (address) = 0 means no block
            continue;
        }
        __u32 cur_logical_offset = logical_offset + address_idx * indirect_offsets[level-1];
        fprintf(stdout, "%s,%d,%d,%d,%u,%u\n",
                "INDIRECT",
                inode_num,
                level,
                cur_logical_offset,
                block_num,
                next_level_bn
                );
        if (level > 1) {
            process_indirect_block(level - 1, next_level_bn, cur_logical_offset, inode_num);
        }
    }
}

// -----------------------------------------------------------------------------

void indirect_block_references_summary() {
    __u32 group_idx;
    for (group_idx = 0; group_idx < sb.n_group; group_idx++) {
        __u32 first_inode_bn = groups[group_idx].first_inode;
        __u32 inode_num = 1;
        __u32 offset = first_inode_bn * BLOCK_SIZE;
        for (; inode_num <= groups[group_idx].n_inodes;
             inode_num++, offset += sizeof(struct ext2_inode)) {
            /* iterate through inodes */
            struct ext2_inode cur_inode;
            if (pread(fs_fd, &cur_inode, sizeof(cur_inode), offset) == -1) {
                err_exit(2, "ERROR in indirect_block_references_summary");
            }
            __u32 single_indirect_bn = cur_inode.i_block[EXT2_IND_BLOCK];
            __u32 double_indirect_bn = cur_inode.i_block[EXT2_DIND_BLOCK];
            __u32 triple_indirect_bn = cur_inode.i_block[EXT2_TIND_BLOCK];
            if (single_indirect_bn != 0) {
                process_indirect_block(1, single_indirect_bn, EXT2_NDIR_BLOCKS, inode_num);
            }
            if (double_indirect_bn != 0) {
                process_indirect_block(2, double_indirect_bn, EXT2_NDIR_BLOCKS + N_BLOCK, inode_num);
            }
            if (triple_indirect_bn != 0) {
                process_indirect_block(3, triple_indirect_bn, EXT2_NDIR_BLOCKS + N_BLOCK + N_BLOCK * N_BLOCK, inode_num);
            }
        }
    }
}

int main(int argc, const char * argv[]) {
    // insert code here...
    if (argc != 2) {
        err_exit(1, "MUST SPEFICY FILE SYSTEM IMAGE");
    }
    const char* fileSystem = argv[1];
    //  In this project, we will provide EXT2 file system images in ordinary files
    if((fs_fd = open(fileSystem, O_RDONLY)) == -1) {
        fprintf(stderr, "ERROR in main: Fail to open the specified file system file.\n");
        exit(1); //  1? 2?
    }
    superblock_summary();
    group_summary();
    free_block_entries_summary();
    free_inode_entries_summary();
    inode_summary();
    directory_entries_summary();
    indirect_block_references_summary();
    return 0;
}
