#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>
#include <libgen.h>
#include <limits.h>

#include "block.h"
#include "tfs.h"

char diskfile_path[PATH_MAX];

// Declare your in-memory data structures here
struct superblock sb;

/*
 * Get available inode number from bitmap
 */
int get_avail_ino() {
	// Step 1: Read inode bitmap from disk
	bitmap_t inode_bitmap = malloc(BLOCK_SIZE);
	if (!inode_bitmap) {
		perror("malloc failed");
		exit(EXIT_FAILURE);
	}
	if (bio_read(sb.i_bitmap_blk, inode_bitmap) == -1) {
		perror("bio_read failed");
		exit(EXIT_FAILURE);
	}
	// Step 2: Traverse inode bitmap to find an available slot
	for (int i = 0; i < MAX_INUM; i++) {
		if (get_bitmap(inode_bitmap, i) == 0) {
			// Step 3: Update inode bitmap and write to disk
			set_bitmap(inode_bitmap, i);
			bio_write(sb.i_bitmap_blk, inode_bitmap);
			free(inode_bitmap);
			return i;
		}
	}
	free(inode_bitmap);
	return -1;
}

/*
 * Get available data block number from bitmap
 */
int get_avail_blkno() {
	// Step 1: Read data block bitmap from disk
	bitmap_t data_bitmap = malloc(BLOCK_SIZE);
	if (!data_bitmap) {
		perror("malloc failed");
		exit(EXIT_FAILURE);
	}
	if (bio_read(sb.d_bitmap_blk, data_bitmap) == -1) {
		perror("bio_read failed");
		exit(EXIT_FAILURE);
	}
	// Step 2: Traverse data block bitmap to find an available slot
	for (int i = 0; i < MAX_DNUM; i++) {
		if (get_bitmap(data_bitmap, i) == 0) {
			// Step 3: Update data block bitmap and write to disk
			set_bitmap(data_bitmap, i);
			bio_write(sb.d_bitmap_blk, data_bitmap);
			free(data_bitmap);
			return i;
		}
	}
	free(data_bitmap);
	return -1;
}

/*
 * inode operations
 */
int readi(uint16_t ino, struct inode* inode) {
	// Step 1: Get the inode's on-disk block number
	int offset = ino * sizeof(struct inode);
	int block_idx = sb.i_start_blk + offset / BLOCK_SIZE;
	// Step 2: Get offset of the inode in the inode on-disk block
	int inblock_offset = offset % BLOCK_SIZE;
	// Step 3: Read the block from disk and then copy into inode structure
	char* block = malloc(BLOCK_SIZE);
	if (!block) {
		perror("malloc failed");
		exit(EXIT_FAILURE);
	}
	if (bio_read(block_idx, block) == -1) {
		perror("bio_read failed");
		exit(EXIT_FAILURE);
	}
	memcpy(inode, block + inblock_offset, sizeof(struct inode));
	free(block);
	return 0;
}

int writei(uint16_t ino, struct inode* inode) {
	// Step 1: Get the block number where this inode resides on disk
	int offset = ino * sizeof(struct inode);
	int block_idx = sb.i_start_blk + offset / BLOCK_SIZE;
	// Step 2: Get the offset in the block where this inode resides on disk
	int inblock_offset = offset % BLOCK_SIZE;
	// Step 3: Write inode to disk
	char* block = malloc(BLOCK_SIZE);
	if (!block) {
		perror("malloc failed");
		exit(EXIT_FAILURE);
	}
	if (bio_read(block_idx, block) == -1) {
		perror("bio_read failed");
		exit(EXIT_FAILURE);
	}
	memcpy(block + inblock_offset, inode, sizeof(struct inode));
	if (bio_write(block_idx, block) == -1) {
		perror("bio_write failed");
		exit(EXIT_FAILURE);
	}
	free(block);
	return 0;
}


/*
 * directory operations
 */
int dir_find(uint16_t ino, const char* fname, size_t name_len, struct dirent* dirent) {
	// Step 1: Call readi() to get the inode using ino (inode number of current directory)
	struct inode dir_inode;
	if (readi(ino, &dir_inode) == -1) {
		perror("readi failed");
		exit(EXIT_FAILURE);
	}
	for (int i = 0; i < 16; i++) {
		if (dir_inode.direct_ptr[i] == 0) continue;
		char* block = malloc(BLOCK_SIZE);
		if (!block) {
			perror("malloc failed");
			exit(EXIT_FAILURE);
		}
		// Step 2: Get data block of current directory from inode
		if (bio_read(sb.d_start_blk + dir_inode.direct_ptr[i], block) == -1) {
			perror("bio_read failed");
			exit(EXIT_FAILURE);
		}
		// Step 3: Read directory's data block and check each directory entry.
		//If the name matches, then copy directory entry to dirent structure
		struct dirent* entries = (struct dirent*)block;
		for (int j = 0; j < BLOCK_SIZE / sizeof(struct dirent); j++) {
			if (entries[j].valid && strncmp(entries[j].name, fname, name_len) == 0) {
				memcpy(dirent, &entries[j], sizeof(struct dirent));
				free(block);
				return 0;
			}
		}
		free(block);
	}
	return -1;
}

int dir_add(struct inode dir_inode, uint16_t f_ino, const char* fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode

	// Step 2: Check if fname (directory name) is already used in other entries

	// Step 3: Add directory entry in dir_inode's data block and write to disk

	// Allocate a new data block for this directory if it does not exist

	// Update directory inode

	// Write directory entry

	return 0;
}

int dir_remove(struct inode dir_inode, const char* fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode

	// Step 2: Check if fname exist

	// Step 3: If exist, then remove it from dir_inode's data block and write to disk

	return 0;
}

/*
 * namei operation
 */
int get_node_by_path(const char* path, uint16_t ino, struct inode* inode) {

	// Step 1: Resolve the path name, walk through path, and finally, find its inode.
	// Note: You could either implement it in a iterative way or recursive way

	return 0;
}

/*
 * Make file system
 */
int tfs_mkfs() {
	// Call dev_init() to initialize (Create) Diskfile
	dev_init(diskfile_path);
	// write superblock information
	struct superblock init_sb;
	memset(&init_sb, 0, sizeof(init_sb));
	init_sb.magic_num = MAGIC_NUM;
	init_sb.max_inum = MAX_INUM;
	init_sb.max_dnum = MAX_DNUM;
	init_sb.i_bitmap_blk = 1;
	init_sb.d_bitmap_blk = 2;
	init_sb.i_start_blk = 3;
	init_sb.d_start_blk = 3 + (MAX_INUM * sizeof(struct inode) + BLOCK_SIZE - 1) / BLOCK_SIZE;
	char block[BLOCK_SIZE];
	memset(block, 0, BLOCK_SIZE);
	memcpy(block, &init_sb, sizeof(struct superblock));
	bio_write(0, block);
	// initialize inode bitmap
	memset(block, 0, BLOCK_SIZE);
	bio_write(init_sb.i_bitmap_blk, block);
	// initialize data block bitmap
	memset(block, 0, BLOCK_SIZE);
	bio_write(init_sb.d_bitmap_blk, block);
	// update bitmap information for root directory
	// update inode for root directory
	return 0;
}


/*
 * FUSE file operations
 */
static void* tfs_init(struct fuse_conn_info* conn) {

	// Step 1a: If disk file is not found, call mkfs
	if (access(diskfile_path, F_OK) == -1) tfs_mkfs();
	dev_open(diskfile_path);
	// Step 1b: If disk file is found, just initialize in-memory data structures
	// and read superblock from disk
	if (bio_read(0, &sb) <= 0) {
		fprintf(stderr, "Failed to read superblock\n");
		exit(EXIT_FAILURE);
	}

	return NULL;
}

static void tfs_destroy(void* userdata) {

	// Step 1: De-allocate in-memory data structures

	// Step 2: Close diskfile

}

static int tfs_getattr(const char* path, struct stat* stbuf) {

	// Step 1: call get_node_by_path() to get inode from path

	// Step 2: fill attribute of file into stbuf from inode

	stbuf->st_mode = S_IFDIR | 0755;
	stbuf->st_nlink = 2;
	time(&stbuf->st_mtime);

	return 0;
}

static int tfs_opendir(const char* path, struct fuse_file_info* fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: If not find, return -1

	return 0;
}

static int tfs_readdir(const char* path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: Read directory entries from its data blocks, and copy them to filler

	return 0;
}


static int tfs_mkdir(const char* path, mode_t mode) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name

	// Step 2: Call get_node_by_path() to get inode of parent directory

	// Step 3: Call get_avail_ino() to get an available inode number

	// Step 4: Call dir_add() to add directory entry of target directory to parent directory

	// Step 5: Update inode for target directory

	// Step 6: Call writei() to write inode to disk


	return 0;
}

static int tfs_rmdir(const char* path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name

	// Step 2: Call get_node_by_path() to get inode of target directory

	// Step 3: Clear data block bitmap of target directory

	// Step 4: Clear inode bitmap and its data block

	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory

	return 0;
}

static int tfs_releasedir(const char* path, struct fuse_file_info* fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}

static int tfs_create(const char* path, mode_t mode, struct fuse_file_info* fi) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name

	// Step 2: Call get_node_by_path() to get inode of parent directory

	// Step 3: Call get_avail_ino() to get an available inode number

	// Step 4: Call dir_add() to add directory entry of target file to parent directory

	// Step 5: Update inode for target file

	// Step 6: Call writei() to write inode to disk

	return 0;
}

static int tfs_open(const char* path, struct fuse_file_info* fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: If not find, return -1

	return 0;
}

static int tfs_read(const char* path, char* buffer, size_t size, off_t offset, struct fuse_file_info* fi) {

	// Step 1: You could call get_node_by_path() to get inode from path

	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: copy the correct amount of data from offset to buffer

	// Note: this function should return the amount of bytes you copied to buffer
	return 0;
}

static int tfs_write(const char* path, const char* buffer, size_t size, off_t offset, struct fuse_file_info* fi) {
	// Step 1: You could call get_node_by_path() to get inode from path

	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: Write the correct amount of data from offset to disk

	// Step 4: Update the inode info and write it to disk

	// Note: this function should return the amount of bytes you write to disk
	return size;
}

static int tfs_unlink(const char* path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name

	// Step 2: Call get_node_by_path() to get inode of target file

	// Step 3: Clear data block bitmap of target file

	// Step 4: Clear inode bitmap and its data block

	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target file in its parent directory

	return 0;
}

static int tfs_truncate(const char* path, off_t size) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}

static int tfs_release(const char* path, struct fuse_file_info* fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}

static int tfs_flush(const char* path, struct fuse_file_info* fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}

static int tfs_utimens(const char* path, const struct timespec tv[2]) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}


static struct fuse_operations tfs_ope = {
	.init = tfs_init,
	.destroy = tfs_destroy,

	.getattr = tfs_getattr,
	.readdir = tfs_readdir,
	.opendir = tfs_opendir,
	.releasedir = tfs_releasedir,
	.mkdir = tfs_mkdir,
	.rmdir = tfs_rmdir,

	.create = tfs_create,
	.open = tfs_open,
	.read = tfs_read,
	.write = tfs_write,
	.unlink = tfs_unlink,

	.truncate = tfs_truncate,
	.flush = tfs_flush,
	.utimens = tfs_utimens,
	.release = tfs_release
};


int main(int argc, char* argv[]) {
	int fuse_stat;

	getcwd(diskfile_path, PATH_MAX);
	strcat(diskfile_path, "/DISKFILE");

	fuse_stat = fuse_main(argc, argv, &tfs_ope, NULL);

	return fuse_stat;
}

