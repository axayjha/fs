// fs.h: File System

#pragma once

#include "afs/disk.h"

#include <stdint.h>

class FileSystem {
public:
    const static uint32_t MAGIC_NUMBER	     = 0xf0f03410;
    const static uint32_t INODES_PER_BLOCK   = 128;
    const static uint32_t POINTERS_PER_INODE = 5;
    const static uint32_t POINTERS_PER_BLOCK = 1024;

private:
    struct SuperBlock {		// Superblock structure
    	uint32_t MagicNumber;	// File system magic number
    	uint32_t Blocks;	// Number of blocks in file system
    	uint32_t InodeBlocks;	// Number of blocks reserved for inodes
    	uint32_t Inodes;	// Number of inodes in file system
    };

    struct Inode {
    	uint32_t Valid;		// Whether or not inode is valid
    	uint32_t Size;		// Size of file
    	uint32_t Direct[POINTERS_PER_INODE]; // Direct pointers
    	uint32_t Indirect;	// Indirect pointer
    };

    union Block {
    	SuperBlock  Super;			    // Superblock
    	Inode	    Inodes[INODES_PER_BLOCK];	    // Inode block
    	uint32_t    Pointers[POINTERS_PER_BLOCK];   // Pointer block
    	char	    Data[Disk::BLOCK_SIZE];	    // Data block
    };

    // TODO: Internal helper functions
    int    load_inode_block(size_t inumber, bool already_loaded=true);
    int    save_inode_block(size_t inumber);
    size_t  find_free();
    int    get_data_addrs(size_t inumber, int* tmp_array);
    
    // TODO: Internal member variables
    int* FS_Bitmap;
    int current_inode_block = 0;
    Disk* FS_Disk;
    Block FS_Inode_Block;
    Block FS_Data_Block;
    uint32_t FS_Blocks;    // Number of blocks in file system
    uint32_t FS_InodeBlocks;   // Number of blocks reserved for inodes
    uint32_t FS_Inodes;    // Number of inodes in file system
public:
    static void debug(Disk *disk);
    static bool format(Disk *disk);


    void print_block_list();

    bool mount(Disk *disk);

    size_t create();
    bool    remove(size_t inumber);
    size_t stat(size_t inumber);

    size_t read(size_t inumber, char *data, size_t length, size_t offset);
    size_t write(size_t inumber, char *data, size_t length, size_t offset);
};

