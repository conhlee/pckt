#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "pckProcess.h"

#include "files.h"

#include "common.h"

void usage() {
    printf("usage: pckt <unpack|pack> <input pck/dir> <output pck/dir>\n");
    exit(1);
}

int main(int argc, char** argv) {
    if (argc < 4)
        usage();

    const char* mode = argv[1];
    const char* inputPath = argv[2];
    const char* outputPath = argv[3];

    if (strcasecmp(mode, "unpack") == 0) {
        FileHandle fileHndl = FileCreateHandle(inputPath);
    
        printf("pckt > unpack: extracting to \"%s/\"..\n", outputPath);

        PckPreprocess(fileHndl.data_u8);

        PckExtractAll(fileHndl.data_u8, outputPath);

        FileDestroyHandle(fileHndl);
    }
    else if (strcasecmp(mode, "pack") == 0) {
        printf("pckt > pack: packing directory \"%s/\"..\n", inputPath);

        GetAllFilesResult r = getAllFiles(inputPath);

        PckBuildFile* buildFiles = (PckBuildFile*)malloc(sizeof(PckBuildFile) * r.fileCount);
        for (unsigned i = 0; i < r.fileCount; i++) {
            buildFiles[i].hndl = FileCreateHandle(r.fullFilenames[i]);
            buildFiles[i].filename = strdup(r.truncatedFilenames[i]);

            free(r.fullFilenames[i]); free(r.truncatedFilenames[i]);
        }
        free(r.fullFilenames); free(r.truncatedFilenames);

        PckEngineVersion engineVersion;
        engineVersion.majorVersion = 4;
        engineVersion.minorVersion = 2;
        engineVersion.patchVersion = 1;

        FileHandle hndl = PckBuild(buildFiles, r.fileCount, engineVersion);

        printf("Writing file..");

        FileWriteHandle(hndl, outputPath);

        LOG_OK;

        free(hndl.data_void);

        for (unsigned i = 0; i < r.fileCount; i++) {
            free(buildFiles[i].hndl.data_void);
            free(buildFiles[i].filename);
        }
        free(buildFiles);
    }
    else
        usage();

    printf("\nAll done!\n");

    return 0;
}