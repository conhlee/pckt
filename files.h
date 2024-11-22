#ifndef FILES_H
#define FILES_H

#include "common.h"

typedef struct {
    union {
        s8* data_s8;
        u8* data_u8;

        void* data_void;
    };
    u64 size;
} FileHandle;

FileHandle FileCreateHandle(const char* path);
void FileDestroyHandle(FileHandle handle);

int FileWriteHandle(FileHandle hndl, const char *path);

#endif // FILES_H