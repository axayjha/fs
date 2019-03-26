// fs.cpp: File System

#include "afs/fs.h"

#include <algorithm>
#include <sys/types.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <cmath>

// Debug file system -----------------------------------------------------------

void FileSystem::debug(Disk *disk) {
    Block block;

    // Read Superblock
    disk->read(0, block.Data);

    printf("SuperBlock:\n");

    if (block.Super.MagicNumber == MAGIC_NUMBER)
        printf("    magic number is valid\n");
    else
        printf("    magic number is not valid\n");

    printf("SuperBlock:\n");
    printf("    %u blocks\n"         , block.Super.Blocks);
    printf("    %u inode blocks\n"   , block.Super.InodeBlocks);
    printf("    %u inodes\n"         , block.Super.Inodes);

    // Read Inode blocks
    Block iblock;
    for (uint32_t i = 1; i <= block.Super.InodeBlocks; i++) { // loop through inode blocks
        disk->read(i, iblock.Data); // increments block reads
        for (uint32_t j = 0; j < INODES_PER_BLOCK; j++) {   // loop though inodes in each block
            Inode inode = iblock.Inodes[j];
            if (inode.Valid) {
                printf("Inode %d:\n", j);
                printf("    size: %u bytes\n", inode.Size);
                printf("    direct blocks:");
                for (uint32_t k = 0; k < POINTERS_PER_INODE; k++) { // looping through inode's direct pointers
                    if (inode.Direct[k]) {
                        printf(" %lu", (unsigned long)(inode.Direct[k]));
                    }
                }
                printf("\n");
                if (inode.Indirect) {
                    printf("    indirect blocks: %lu\n", (unsigned long)(inode.Indirect));
                    printf("    indirect data blocks:");
                    Block Indirect_block;
                    disk->read(inode.Indirect, Indirect_block.Data);
                    for (uint32_t l = 0; l < POINTERS_PER_BLOCK; l++) {
                        if (Indirect_block.Pointers[l])
                            printf(" %d", (Indirect_block.Pointers[l]));
                    }
                    printf("\n");
                }

            }
        }
    }
}

// Format file system ----------------------------------------------------------

bool FileSystem::format(Disk *disk) {

    // check if disk is mounted
     if (disk->mounted())
        return false;

    int block_index = 0;

    // Write superblock
    Block block;
    block.Super.MagicNumber = MAGIC_NUMBER;
    block.Super.Blocks = disk->size();
    block.Super.InodeBlocks = std::ceil(block.Super.Blocks * 0.10);
    block.Super.Inodes = block.Super.InodeBlocks * INODES_PER_BLOCK;
    disk->write(block_index, block.Data);
    block_index++;

    /* Check this by manually running test for outputs ^^^^^^^^^^^^^^^^^^^^^^*/
    //loop through inodeblocks and write to each
    for (uint32_t i = block_index; i <= block.Super.InodeBlocks; i++) {
        //Clear inodes in inode block
        for (uint32_t j = 0; j < INODES_PER_BLOCK; j++) {
            block.Inodes[j].Valid = 0;
            block.Inodes[j].Size = 0;
            for (uint32_t k = 0; k < POINTERS_PER_INODE; k++) {
                block.Inodes[j].Direct[k] = 0;
            }
            block.Inodes[j].Indirect = 0;
        }
        disk->write(block_index, block.Data);
        block_index++;
    }
    //Clear pointers within indirect block
    for (uint32_t i = 0; i < POINTERS_PER_BLOCK; i++) {
        block.Pointers[i] = 0;
    }
    disk->write(block_index, block.Data);
    block_index++;

    //Clear blocks in data block
    for (uint32_t i = 0; i < Disk::BLOCK_SIZE; i++) {
        block.Data[i] = 0;
    }

    //Write to the remaining data blocks
    for (uint32_t i = block_index; i < disk->size(); i++) {
        disk->write(block_index, block.Data);
        block_index++;
    }

    return true;
}

// Mount file system -----------------------------------------------------------

bool FileSystem::mount(Disk *disk) {

    // make sure no device is mounted
    if (Device)
        return false;

    // make sure no disk is mountedd
    if (disk->mounted())
        return false;

    // Read superblock
    Block block;
    disk->read(0, block.Data);

    // check if disk has valid superblock metadata
    if (block.Super.MagicNumber != MAGIC_NUMBER)    // check magic number
        return false;
    if (block.Super.Blocks != disk->size())     // check blocks
        return false;
    if (block.Super.InodeBlocks != std::ceil(block.Super.Blocks * 0.10))  //  Check 10%
        return false;
    if (block.Super.Inodes != block.Super.InodeBlocks * INODES_PER_BLOCK)  //  Check Inodes
        return false;

    // Set device and mount
    Device = disk;
    Device->mount();

    // Copy metadata
    super_block = block.Super;

    // Allocate free block bitmap

    //go through and read the blocks and store the whether it is allocated in free block bitmap
    for (uint32_t i = 1; i <= block.Super.InodeBlocks; i++) {  //  Loop through inode blocks.
        disk->read(i, block.Data);
        for (uint32_t j = 0; j < INODES_PER_BLOCK; j++) {      //  Loop through inodes in each block.
            Inode inode = block.Inodes[j];
            if (inode.Valid) {
                for (uint32_t k = 0; k < POINTERS_PER_INODE; k++) { 
                    if (inode.Direct[k]) {
                        free_block_bitmap.emplace(inode.Direct[k], 1); //1 for allocated
                    }
                }
                if (inode.Indirect) { 
                    free_block_bitmap.emplace(inode.Indirect, 1);
                    Block Indirect_block;
                    disk->read(inode.Indirect, Indirect_block.Data);
                    for (uint32_t l = 0; l < POINTERS_PER_BLOCK; l++) { // Loop throught pointers from indirect block
                        if (Indirect_block.Pointers[l])
                            free_block_bitmap.emplace(Indirect_block.Pointers[l], 1);
                    }
                } 
            }
        }
    }

    return true;
}

// Create inode ----------------------------------------------------------------

ssize_t FileSystem::create() {
    // Locate free inode in inode table
    Block block;
    for (uint32_t i = 1; i <= super_block.InodeBlocks; i++) {  //  Loop through inode blocks.
        Device->read(i, block.Data);
        for (uint32_t j = 0; j < INODES_PER_BLOCK; j++) {      //  Loop through inodes in each block.
            if (!block.Inodes[j].Valid) {
                block.Inodes[j].Valid = 1;
                Device->write(i, block.Data);
                return ((i - 1) * INODES_PER_BLOCK) + j; //return the exact block inumber
            }
        }
    }
    return -1;
}

// Remove inode ----------------------------------------------------------------

bool FileSystem::remove(size_t inumber) {
    // Load inode information

    // Free direct blocks

    // Free indirect blocks

    // Clear inode in inode table
    return true;
}

// Inode stat ------------------------------------------------------------------

ssize_t FileSystem::stat(size_t inumber) {
    // Load inode information
    return 0;
}

// Read from inode -------------------------------------------------------------

ssize_t FileSystem::read(size_t inumber, char *data, size_t length, size_t offset) {
    // Load inode information

    // Adjust length

    // Read block and copy to data
    return 0;
}

// Write to inode --------------------------------------------------------------

ssize_t FileSystem::write(size_t inumber, char *data, size_t length, size_t offset) {
    // Load inode
    
    // Write block and copy to data
    return 0;
}
