#include "files.h"

#include <stdlib.h>
#include <stdio.h>

FileHandle FileCreateHandle(const char* path) {
    FileHandle hndl;

    FILE* fp = fopen(path, "rb");
    if (fp == NULL)
        panic("FileCreateHandle: fopen failed (path : %s)", path);

    fseek(fp, 0, SEEK_END);
    hndl.size = ftell(fp);
    rewind(fp);

    hndl.data_void = malloc(hndl.size);
    if (hndl.data_void == NULL) {
        fclose(fp);

        panic("FileCreateHandle: malloc failed");
    }

    u64 bytesCopied = fread(hndl.data_void, 1, hndl.size, fp);

    fclose(fp);

    if (bytesCopied != hndl.size) {
        free(hndl.data_void);

        panic("FileCreateHandle: fread failed (path : %s)", path);
    }

    return hndl;
}
void FileDestroyHandle(FileHandle handle) {
    if (handle.data_void)
        free(handle.data_void);
}

int FileWriteHandle(FileHandle hndl, const char *path) {
    FILE* fp = fopen(path, "wb");
    if (fp == NULL) {
        warn("FileWriteHandle: fopen failed (path : %s)", path);
        return 1;
    }

    u64 bytesCopied = fwrite(hndl.data_void, 1, hndl.size, fp);

    fclose(fp);

    if (bytesCopied != hndl.size)
        panic("FileCreateHandle: fwrite failed (path : %s)", path);

    return 0;
}