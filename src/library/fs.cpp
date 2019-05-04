// fs.cpp: File System

#include "afs/fs.h"

#include <algorithm>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>

int FileSystem::get_data_addrs(size_t inumber, int *tmp_array){
    int tmp_addr;

    int tmp_index = load_inode_block(inumber, false);
    for(int i = 0; i < 1029; i++){
        tmp_array[i] = 0;
    }

    for(int i = 0; i < 5; i++){
        tmp_addr = FS_Inode_Block.Inodes[tmp_index].Direct[i];
        tmp_array[i] = tmp_addr;

    }

    if(FS_Inode_Block.Inodes[tmp_index].Valid == false){
        return 0;
    }
    uint32_t indirect_add = FS_Inode_Block.Inodes[tmp_index].Indirect;
    if((indirect_add == 0) || (indirect_add > FS_Blocks)){
        return 0;
    }
    FS_Bitmap[indirect_add] = 1;
    Block indirectBlock;

    FS_Disk->read(indirect_add, indirectBlock.Data);
    for(uint32_t i = 0; i < POINTERS_PER_BLOCK; i++){
        tmp_addr = indirectBlock.Pointers[i];
        tmp_array[5+i] = tmp_addr;
        if(tmp_addr == 0){
            return 0;
        }

    }

    return 0;
}

size_t FileSystem::find_free(){
    for(int i= 0 ; i < int(FS_Blocks) ; i++){
        if(FS_Bitmap[i] == 0){
            return (size_t) i;
        }
    }

    return -1;
}

void FileSystem::print_block_list(){
    for(int i = 0 ; i < FS_Blocks; i++){
        printf("[%d] %u \n "  , i, FS_Bitmap[i]);    
    }
}

int FileSystem::load_inode_block(size_t inumber, bool already_loaded){
    int tmp_index = inumber;
    int tmp_inode_block = 1;

    if(tmp_index >= (int) INODES_PER_BLOCK){
        tmp_index = tmp_index - INODES_PER_BLOCK;
        tmp_inode_block++;
    }

    //if(tmp_inode_block != current_inode_block){
        //current_inode_block = tmp_inode_block;
        // TODO, return false if the index is smaller than 0, or larger than the number of inodes 
        if(already_loaded == false){
            FS_Disk->read(tmp_inode_block, FS_Inode_Block.Data);
        }
    //}
    if(tmp_index >= 0 && tmp_index < INODES_PER_BLOCK){
        return tmp_index;
    }else{
        return -1;
    }
}

int FileSystem::save_inode_block(size_t inumber){
    int tmp_index = inumber;
    int tmp_inode_block = 1;

    if(tmp_index >= (int) INODES_PER_BLOCK){
        tmp_index = tmp_index - INODES_PER_BLOCK;
        tmp_inode_block++;
    }

    // TODO, return false if the index is smaller than 0, or larger than the number of inodes 
    FS_Disk->write(tmp_inode_block, FS_Inode_Block.Data);

    return 0;
}
// Debug file system -----------------------------------------------------------

void FileSystem::debug(Disk *disk) {
    Block block;

    // Read Superblock
    disk->read(0, block.Data);

    printf("SuperBlock:\n");
    if (block.Super.MagicNumber == MAGIC_NUMBER) {
	    printf("    magic number is valid\n");
    } 
    else {
        printf("    magic number is invalid\n");
    }
    printf("    %u blocks\n"         , block.Super.Blocks);
    printf("    %u inode blocks\n"   , block.Super.InodeBlocks);
    printf("    %u inodes\n"         , block.Super.Inodes);



    // Read Inode blocks
    Block inode_block, pointer_block;
    bool need_indirect = true;
    // For Each Inode Block
    for (uint32_t k = 1; k <= block.Super.InodeBlocks; k++) {
        disk->read(k, inode_block.Data);

        // For each Inode 
        for (uint32_t i = 0; i < INODES_PER_BLOCK; i++) {
            if (inode_block.Inodes[i].Valid){

                printf("Inode %d:\n", i*k);
                printf("    size: %u bytes\n" , inode_block.Inodes[i].Size);

                // For each of the pointers in the inode
                printf("    direct blocks:");
                for (uint32_t j = 0; j < POINTERS_PER_INODE; j++) {
                    if (inode_block.Inodes[i].Direct[j]) {
                        //if(j != 0) printf(" ");
                        printf(" %u",inode_block.Inodes[i].Direct[j]);
                    }
                    else{
                        need_indirect = false;
                        break;
                    }
                }
                printf("\n");

		// indirect blocks
                if(need_indirect){
                    size_t indirect_addr = inode_block.Inodes[i].Indirect;
                    printf("    indirect block: %u\n", indirect_addr);
                    disk->read(indirect_addr, pointer_block.Data);

		            printf("    indirect data blocks:");
                    for(int j = 0; j < POINTERS_PER_BLOCK; j++){
                        if(pointer_block.Pointers[j] != 0){
                            //if(j != 0) printf(" ");
                            printf(" %u",pointer_block.Pointers[j]);
                        }
                        else{
                            break;
                        }
                    }
                    printf("\n");
                }
		// reset flag
		need_indirect = true;
            }
        }
    }   
}

// Format file system ----------------------------------------------------------

bool FileSystem::format(Disk *disk) {
    // check if already mounted, you can't format so return false
    if (disk->mounted()) return false;

    //Block old_super;
    //disk->unmount();
    //disk->read(0,old_super.Data);

    size_t fs_size = disk->size();
    size_t tmp_inode_data_pointer = fs_size / 10;

    if(tmp_inode_data_pointer == 0) tmp_inode_data_pointer++;


    Block block;
    block.Super.MagicNumber = MAGIC_NUMBER;
    block.Super.Blocks = fs_size;
    block.Super.InodeBlocks = tmp_inode_data_pointer;
    block.Super.Inodes = block.Super.InodeBlocks * INODES_PER_BLOCK;
    //Block new_super;
    //new_super.Super.MagicNumber = old_super.Super.MagicNumber;
    //new_super.Super.Blocks = fs_size;
    //new_super.Super.InodeBlocks = tmp_inode_data_pointer;
    //new_super.Super.Inodes = 0;

    // Write superblock
    disk->write(0,block.Data);
    //disk->write(0,new_super.Data);

    // Clear all other blocks
    Block tmp_inode_block;

    for (size_t i = 0; i < Disk::BLOCK_SIZE; i++) {
	tmp_inode_block.Data[i] = 0;
    }
    for (size_t j = 1; j < fs_size; j++) {
	disk->write(j, tmp_inode_block.Data);
    }	
    /*
    // For each of the blocks which hold inode information
    for(int i = 1; i <= int(tmp_inode_data_pointer); i++){
        // Reset the blocks inode info
        for(int j = 0; j < int(INODES_PER_BLOCK); j++){
            Inode tmp_inode;
            tmp_inode.Valid = false;
            tmp_inode.Size = 0;
            tmp_inode.Indirect = 0;
            for(int i = 0; i < 5; i++){
                tmp_inode.Direct[i] = 0;
            }
            tmp_inode_block.Inodes[j] = tmp_inode;
        }
        // And write the new block to the correct position 
        disk->write(i,tmp_inode_block.Data);
    }
    */

    return true;
}

// Mount file system -----------------------------------------------------------

bool FileSystem::mount(Disk *disk) {
    // look if filesystem is present
    if (disk->mounted()) return false;   
 
    // Read superblock
    disk->read(0,FS_Data_Block.Data);

    // BAD MOUNT 1 & 2, Incorrect Magic Number
    if(FS_Data_Block.Super.MagicNumber != MAGIC_NUMBER) return false;

    // BAD MOUNT 3, No Blocks
    if(FS_Data_Block.Super.Blocks == 0) return false;  

    // BAD MOUNT 4, Too Many Inode Blocks 
    if(FS_Data_Block.Super.InodeBlocks*INODES_PER_BLOCK > FS_Data_Block.Super.Inodes) return false;

    // BAD MOUNT 5, Not Enough Inodes For the Number of Inode Blocks 
    if(FS_Data_Block.Super.Inodes != FS_Data_Block.Super.InodeBlocks*INODES_PER_BLOCK) return false;
    // Set device and mount

    FS_Disk = disk;
    disk->mount();

    // Copy metadata
    FS_Blocks = FS_Data_Block.Super.Blocks;             // Total Number of blocks
    FS_InodeBlocks = FS_Data_Block.Super.InodeBlocks;   // Number of inode blocks
    FS_Inodes = FS_Data_Block.Super.Inodes;             // Number of inodes 

    // Allocate free block bitmap & Initialize Values
    FS_Bitmap = new int[FS_Blocks];
    for(uint32_t i = 0 ; i < FS_Blocks; i++){
        if(i <= FS_InodeBlocks){
            FS_Bitmap[i] = 1;
        }else{
            FS_Bitmap[i] = 0;
        }
    }

    // Update the Bitmap for every address pointed to in an inode 
    int tmp_index = 1;
    FS_Disk->read(tmp_index, FS_Inode_Block.Data);

    for(uint32_t i = 0 , x=0; i < FS_Inodes; i++, x++){
        if(x == INODES_PER_BLOCK){
            tmp_index++;
            x = 0;
            FS_Disk->read(tmp_index, FS_Inode_Block.Data);
        }

        if(FS_Inode_Block.Inodes[x].Valid){ 
            for(uint32_t j = 0 ; j < 5 ; j++){
                if(FS_Inode_Block.Inodes[x].Direct[j] != 0){
                    FS_Bitmap[FS_Inode_Block.Inodes[x].Direct[j]] = 1;
                }  
            }
            if(FS_Inode_Block.Inodes[x].Indirect != 0){
                FS_Disk->read(FS_Inode_Block.Inodes[x].Indirect, FS_Inode_Block.Data);
                for(uint32_t j = 0 ; j < POINTERS_PER_BLOCK ; j++){
                    if(FS_Inode_Block.Pointers[j] != 0){
                        FS_Bitmap[FS_Inode_Block.Pointers[j]] = 1;
                    }
                }
                FS_Disk->read(tmp_index, FS_Inode_Block.Data);
            }
        }
    }

    //print_block_list();

    return true;
}

// Create inode ----------------------------------------------------------------

size_t FileSystem::create() {
    // Locate free inode in inode table
    int inode = 0;
    int tmp_index;
    load_inode_block(0, false);
    for(int i = 0 ; i < FS_Inodes ; i++){
        tmp_index = load_inode_block(i);
        if(FS_Inode_Block.Inodes[tmp_index].Valid == 0){
            tmp_index = load_inode_block(i, false);
            // Reset All of It's Data, Make It Valid, and Save this info to the inode block
            FS_Inode_Block.Inodes[i].Valid = 1;
            FS_Inode_Block.Inodes[i].Indirect = 0;
            FS_Inode_Block.Inodes[i].Size = 0;
            for(int j = 0 ; j < POINTERS_PER_INODE ; j++){
                FS_Inode_Block.Inodes[i].Direct[j] = 0;
            }

            save_inode_block(i);
         
            // Return the inode # of the found inode. 
            return i; 
        }
    }
    //print_block_list();   
    // Record inode if found
    return -1;
}

// Remove inode ----------------------------------------------------------------

bool FileSystem::remove(size_t inumber) {
    // Load inode information
    int tmp_index = load_inode_block(inumber, false);  
    if(FS_Inode_Block.Inodes[tmp_index].Valid == 0){
       return false;
    }

    // Set the Inode Valid Bit to 0 & save the information 
    FS_Inode_Block.Inodes[tmp_index].Valid = 0;
    save_inode_block(inumber);

    // Set the value of each data block for the inode to 0 in the bitmap
    int data_addrs[1029];
    get_data_addrs(inumber, &data_addrs[0]);
    for(int j = 0; data_addrs[j] != 0 ; j++){
        FS_Bitmap[data_addrs[j]] = 0;
    }


    //print_block_list();
    return true;
}

// Inode stat ------------------------------------------------------------------

size_t FileSystem::stat(size_t inumber) {

    // Load inode information
    current_inode_block = 0;
    int tmp_index = load_inode_block(inumber, false);

    if(tmp_index != -1){
        if(FS_Inode_Block.Inodes[tmp_index].Valid != 0){
           return FS_Inode_Block.Inodes[tmp_index].Size;
        }
    }

    return -1;
}

// Read from inode -------------------------------------------------------------

size_t FileSystem::read(size_t inumber, char *data, size_t length, size_t offset) {
    //printf("PREFORMING A READ of inode %lu, length %lu, starting at offset %lu\n", inumber, length, offset);

    int this_length, tmp_index, block_offset, data_block_index, data_pointer;
    size_t bytes_copied;
    size_t real_length = length;
    int data_addrs[1029];

    get_data_addrs(inumber, &data_addrs[0]);
    tmp_index = load_inode_block(inumber);
    block_offset = offset % 4096;
    data_block_index = offset / 4096;

    if(FS_Inode_Block.Inodes[tmp_index].Valid != 0){
 
        if(offset + length > FS_Inode_Block.Inodes[tmp_index].Size){
            real_length = FS_Inode_Block.Inodes[tmp_index].Size - offset;
            if(real_length == 0){
                return -1;
            }
        }

        bytes_copied = 0;
        while(bytes_copied < real_length){

            data_pointer = data_addrs[data_block_index];  
            // If the direct pointer points to a data block
            if(data_pointer != 0){

                if(block_offset + real_length - bytes_copied <= 4096){
                    this_length = real_length - bytes_copied;
                } 
                else {
                    // If for the starting block, the length goes past the bounds of the block...
                    this_length = 4096 - block_offset;
                    block_offset = 0;
                }

                FS_Disk->read(data_pointer, FS_Data_Block.Data);
                strncpy(data, &FS_Data_Block.Data[block_offset], this_length);           
                bytes_copied = bytes_copied + this_length;
            }
            else{
                break;
            }
            //Go to next data block
            data_block_index++;
        }
        return bytes_copied;

    }

    // Read block and copy to data
    return -1;
}

// Write to inode --------------------------------------------------------------

size_t FileSystem::write(size_t inumber, char *data, size_t length, size_t offset) {

    //printf("PREFORMING A WRITE of inode %lu, length %lu, starting at offset %lu\n", inumber, length, offset);

    int this_length, tmp_index, block_offset, data_block_index, data_pointer;
    size_t bytes_copied;
    int data_addrs[1029];

    get_data_addrs(inumber, &data_addrs[0]);
    tmp_index = load_inode_block(inumber);
    data_block_index = offset / 4096;
    block_offset = offset % 4096;

    // IF THE INODE IS VALID
    if(FS_Inode_Block.Inodes[tmp_index].Valid != 0){


        bytes_copied = 0;
        while(bytes_copied < length){
 
            data_pointer = data_addrs[data_block_index];
            if(data_pointer != 0){

                if(block_offset + length - bytes_copied <= 4096){
                    this_length = length - bytes_copied;
                } 
                else {
                    this_length = 4096 - block_offset;
                    block_offset = 0;
                }

                FS_Disk->read(data_pointer, FS_Data_Block.Data);
                strncpy(&FS_Data_Block.Data[block_offset], data, this_length);           
                FS_Disk->write(data_pointer, FS_Data_Block.Data);

                bytes_copied = bytes_copied + this_length;
                data_block_index++;
            } else{
                // IF THERE IS NOT A DATA BLOCK POINTED TO CREATE ONE, UPDATE SIZE, DO NOT INCREMENT LOOP
                size_t open_block = find_free();
                if(open_block == -1){
                    return bytes_copied;
                }

                // Update the Address array, Bitmap
                data_addrs[data_block_index] = open_block;
                FS_Bitmap[open_block] = 1;

                //Update the Inode Info
                // BROKEN, NEED TO BE ABLE TO ADD TO INFO TO INDIRECT BLOCK 
                FS_Inode_Block.Inodes[tmp_index].Direct[data_block_index] = open_block;
                if(block_offset + length <= 4096){
                    FS_Inode_Block.Inodes[tmp_index].Size = FS_Inode_Block.Inodes[tmp_index].Size + block_offset + length - bytes_copied;
                } 
                else {
                    FS_Inode_Block.Inodes[tmp_index].Size = FS_Inode_Block.Inodes[tmp_index].Size + 4096 - block_offset;
                }
                save_inode_block(inumber); 
            }
                      
        }

        return bytes_copied;

    }

    // Read block and copy to data
    return -1;
}

