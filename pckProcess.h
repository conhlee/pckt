#ifndef PCKPROCESS_H
#define PCKPROCESS_H

#include "files.h"

#include "common.h"

typedef struct __attribute((packed)) {
    u32 majorVersion;
    u32 minorVersion;
    u32 patchVersion;
} PckEngineVersion;

void PckPreprocess(u8* pckData);

void PckExtractAll(u8* pckData, const char* outputDirectory);

typedef struct {
    char* filename; // Not including root directory
    FileHandle hndl;
} PckBuildFile;

FileHandle PckBuild(PckBuildFile* files, u32 fileCount, PckEngineVersion engineVersion);

#endif // PCKPROCESS_H
