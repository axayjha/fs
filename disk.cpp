// disk.h: Disk emulator

#include <stdexcept>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <stdlib.h>
#include <sys/types.h>


#include <iostream>

using namespace std;
class Disk {
private:
    int	    FileDescriptor;  // File descriptor of disk image
    size_t  Blocks;          // Number of blocks in disk image
    size_t  Reads;           // Number of reads performed
    size_t  Writes;          // Number of writes performed
    size_t  Mounts;          // Number of mounts

    // Check parameters
    // @param	blocknum    Block to operate on
    // @param	data	    Buffer to operate on
    // Throws invalid_argument exception on error.
    void sanity_check(int blocknum, char *data);

public:
    // Number of bytes per block
    const static size_t BLOCK_SIZE = 4096;
    
    // Default constructor
    Disk() : FileDescriptor(0), Blocks(0), Reads(0), Writes(0), Mounts(0) {}
    
    // Destructor
    ~Disk();

    // Open disk image
    // @param	path	    Path to disk image
    // @param	nblocks	    Number of blocks in disk image
    // Throws runtime_error exception on error.
    void open(const char *path, size_t nblocks);

    // Return size of disk (in terms of blocks)
    size_t size() const { return Blocks; }

    // Return whether or not disk is mounted
    bool mounted() const { return Mounts > 0; }

    // Increment mounts
    void mount() { Mounts++; }

    // Decrement mounts
    void unmount() { if (Mounts > 0) Mounts--; }

    // Read block from disk
    // @param	blocknum    Block to read from
    // @param	data	    Buffer to read into
    void read(int blocknum, char *data);
    
    // Write block to disk
    // @param	blocknum    Block to write to
    // @param	data	    Buffer to write from
    void write(int blocknum, char *data);
};


void Disk::open(const char *path, size_t nblocks) {
    FileDescriptor = ::open(path, O_RDWR|O_CREAT, 0600);
    if (FileDescriptor < 0) {
        char what[BUFSIZ];
        snprintf(what, BUFSIZ, "Unable to open %s: %s", path, strerror(errno));
        throw std::runtime_error(what);
    }

    if (ftruncate(FileDescriptor, nblocks*BLOCK_SIZE) < 0) {
        char what[BUFSIZ];
        snprintf(what, BUFSIZ, "Unable to open %s: %s", path, strerror(errno));
        throw std::runtime_error(what);
    }

    Blocks = nblocks;
    Reads  = 0;
    Writes = 0;
}

Disk::~Disk() {
    if (FileDescriptor > 0) {
        printf("%lu disk block reads\n", Reads);
        printf("%lu disk block writes\n", Writes);
        close(FileDescriptor);
        FileDescriptor = 0;
    }
}

void Disk::sanity_check(int blocknum, char *data) {
    char what[BUFSIZ];

    if (blocknum < 0) {
        snprintf(what, BUFSIZ, "blocknum (%d) is negative!", blocknum);
        throw std::invalid_argument(what);
    }

    if (blocknum >= (int)Blocks) {
        snprintf(what, BUFSIZ, "blocknum (%d) is too big!", blocknum);
        throw std::invalid_argument(what);
    }

    if (data == NULL) {
        snprintf(what, BUFSIZ, "null data pointer!");
        throw std::invalid_argument(what);
    }
}

void Disk::read(int blocknum, char *data) {
    sanity_check(blocknum, data);

    if (lseek(FileDescriptor, blocknum*BLOCK_SIZE, SEEK_SET) < 0) {
        char what[BUFSIZ];
        snprintf(what, BUFSIZ, "Unable to lseek %d: %s", blocknum, strerror(errno));
        throw std::runtime_error(what);
    }

    if (::read(FileDescriptor, data, BLOCK_SIZE) != BLOCK_SIZE) {
        char what[BUFSIZ];
        snprintf(what, BUFSIZ, "Unable to read %d: %s", blocknum, strerror(errno));
        throw std::runtime_error(what);
    }

    Reads++;
}

void Disk::write(int blocknum, char *data) {
    sanity_check(blocknum, data);

    if (lseek(FileDescriptor, blocknum*BLOCK_SIZE, SEEK_SET) < 0) {
        char what[BUFSIZ];
        snprintf(what, BUFSIZ, "Unable to lseek %d: %s", blocknum, strerror(errno));
        throw std::runtime_error(what);
    }

    if (::write(FileDescriptor, data, BLOCK_SIZE) != BLOCK_SIZE) {
        char what[BUFSIZ];
        snprintf(what, BUFSIZ, "Unable to write %d: %s", blocknum, strerror(errno));
        throw std::runtime_error(what);
    }

    Writes++;
}




int main()
{
    char *path = (char *)malloc(100*sizeof(char));
    strcpy(path, "0xf0f03410");
    cout << "Hello\n";
    Disk *dev = new Disk();
    dev->open("image.10", 10);
    dev->write(0, path);
}

