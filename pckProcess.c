#include "pckProcess.h"

#include <stdlib.h>

#include <string.h>

#include <openssl/evp.h>

#define GDPC_MAGIC IDENTIFIER_TO_U32('G', 'D', 'P', 'C')
#define PCK_VERSION (2)

#define PCK_FLAG_DIR_ENCRYPTED (1 << 0)
#define PCK_FLAG_REL_FILEBASE (1 << 1)

#define PCK_ALIGNMENT (4)

typedef struct __attribute((packed)) {
    u32 size;
    char content[0];
} PckString;

typedef struct __attribute((packed)) {
    u64 dataOffset; // Relative to fileHeader->dataSectionOffset
    u64 dataSize;
    unsigned char md5Hash[16];
    u32 flags;
} PckFileEntry;

typedef struct __attribute((packed)) {
    u32 magic; // Compare to GDPC_MAGIC

    u32 formatVersion; // Compare to PCK_VERSION

    PckEngineVersion engineVersion;

    u32 flags; // PCK_FLAG_DIR_ENCRYPTED, PCK_FLAG_REL_FILEBASE

    u64 dataSectionOffset;

    u32 reserved[16];

    u32 fileCount;
} PckFileHeader;

static unsigned _PckAlignUint(unsigned value) {
    return (value + PCK_ALIGNMENT - 1) & ~(PCK_ALIGNMENT - 1);
}

typedef void* MD5Context;

static MD5Context _PckCreateMD5Context() {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (context == NULL)
        panic("_PckCreateMD5Context: EVP_MD_CTX_new failed");

    if (EVP_DigestInit_ex(context, EVP_md5(), NULL) != 1) {
        EVP_MD_CTX_free(context);
        panic("_PckCreateMD5Context: EVP_DigestInit_ex failed");
    }

    return (MD5Context)context;
}

static void _PckEndMD5Context(MD5Context context) {
    EVP_MD_CTX_free((EVP_MD_CTX*)context);
}

static void _PckProduceMD5(void* data, u64 size, MD5Context context, unsigned char* hashOutput) {
    if (EVP_DigestInit_ex((EVP_MD_CTX*)context, EVP_md5(), NULL) != 1)
        panic("_PckProduceMD5: EVP_DigestInit_ex failed");

    if (EVP_DigestUpdate((EVP_MD_CTX*)context, data, size) != 1)
        panic("_PckProduceMD5: EVP_DigestUpdate failed");

    unsigned mdl;
    if (EVP_DigestFinal_ex((EVP_MD_CTX*)context, hashOutput, &mdl) != 1)
        panic("_PckProduceMD5: EVP_DigestFinal_ex failed");

    if (mdl != 16)
        panic("_PckProduceMD5: MD5 digest final size mismatch");
}

// stringRep must be at least 32 bytes
static void _PckMD5ToString(const unsigned char* md5Hash, char* stringRep) {
    for (unsigned i = 0; i < 16; i++)
        sprintf(stringRep + (i * 2), "%02x", md5Hash[i]);
}

void PckPreprocess(u8* pckData) {
    PckFileHeader* fileHeader = (PckFileHeader*)pckData;

    if (fileHeader->magic != GDPC_MAGIC)
        panic("PckPreprocess: PCK magic is nonmatching");

    if (fileHeader->formatVersion != PCK_VERSION)
        panic("PckPreprocess: Expected PCK version %u, got %u instead", PCK_VERSION, fileHeader->formatVersion);
    
    if (fileHeader->flags & PCK_FLAG_DIR_ENCRYPTED)
        panic("PckPreprocess: Encrypted PCK not supported");
}

static void _PckFormatSize(unsigned sizeInBytes, char* stringRep) {
    if (sizeInBytes < 1024)
        sprintf(stringRep, "%ub", sizeInBytes);
    else
        sprintf(stringRep, "%ukb", sizeInBytes / 1024);
}

void PckExtractAll(u8* pckData, const char* outputDirectory) {
    PckFileHeader* fileHeader = (PckFileHeader*)pckData;

    PckString* curName = (PckString*)(fileHeader + 1);
    PckFileEntry* curEntry;

    printf("Extracting %u files: \n", fileHeader->fileCount);

    MD5Context md5Context = _PckCreateMD5Context();

    for (unsigned i = 0; i < fileHeader->fileCount; i++) {
        curEntry = (PckFileEntry*)(
            (u8*)curName +
            _PckAlignUint(sizeof(PckString) + curName->size)
        );

        char sizeStr[32];
        _PckFormatSize(curEntry->dataSize, sizeStr);

        printf("    - \"%.*s\" (%s)..", (int)curName->size, curName->content, sizeStr);

        FileHandle hndl;
        hndl.data_u8 = pckData + fileHeader->dataSectionOffset + curEntry->dataOffset;
        hndl.size = curEntry->dataSize;

        // Check MD5 hash
        unsigned char hash[16];
        _PckProduceMD5(hndl.data_void, hndl.size, md5Context, hash);

        if (memcmp(hash, curEntry->md5Hash, 16) != 0) {
            char hashA[32]; _PckMD5ToString(curEntry->md5Hash, hashA);
            char hashB[32]; _PckMD5ToString(hash, hashB);

            panic(
                "PckExtractAll: MD5 hash is nonmatching\n"
                "    - Expected: %.32s\n"
                "    - Got: %.32s",
                hashA, hashB
            );
        }

        // Construct path
        char fullPath[256];
        sprintf(
            fullPath,
            "%s" PATH_SEPARATOR_S "%.*s",
            outputDirectory,

            // Remove res:// from filename
            (int)(curName->size - STR_LIT_LEN("res://")),
            curName->content + STR_LIT_LEN("res://")
        );

        char directoryPath[256];
        sprintf(
            directoryPath, "%.*s",
            (int)(getFilename(fullPath) - fullPath), fullPath    
        );

        createDirectoryTree(directoryPath);

        // Write file
        FileWriteHandle(hndl, fullPath);

        LOG_OK;

        curName = (PckString*)(curEntry + 1);
    }

    _PckEndMD5Context(md5Context);
}

FileHandle PckBuild(PckBuildFile* files, u32 fileCount, PckEngineVersion engineVersion) {
    FileHandle resultHndl;

    printf("Precomputing PCK size..");

    unsigned entrySectionSize = 0;
    for (unsigned i = 0; i < fileCount; i++) {
        entrySectionSize += sizeof(PckString) + _PckAlignUint(
            strlen(files[i].filename) + STR_LIT_LEN("res://")
        );
        entrySectionSize += sizeof(PckFileEntry);
    }

    unsigned dataSectionSize = 0;
    for (unsigned i = 0; i < fileCount; i++)
        dataSectionSize += _PckAlignUint(files[i].hndl.size);

    const unsigned fileSize = sizeof(PckFileHeader) + entrySectionSize + dataSectionSize;

    resultHndl.data_void = malloc(fileSize);
    if (!resultHndl.data_void)
        panic("PckBuild: malloc failed");
    
    resultHndl.size = fileSize;

    LOG_OK;

    printf("Writing %u files: \n", fileCount);

    PckFileHeader* fileHeader = (PckFileHeader*)resultHndl.data_void;

    fileHeader->magic = GDPC_MAGIC;
    fileHeader->formatVersion = PCK_VERSION;
    fileHeader->engineVersion = engineVersion;
    fileHeader->flags = 0x00000000;
    fileHeader->dataSectionOffset = sizeof(PckFileHeader) + entrySectionSize;
    fileHeader->fileCount = fileCount;

    memset(fileHeader->reserved, 0x00, sizeof(fileHeader->reserved));

    u8* const entrySectionStart = (u8*)(fileHeader + 1);
    u8* const dataSectionStart  = entrySectionStart + entrySectionSize;

    u8* nextEntry = entrySectionStart;
    u8* nextData = dataSectionStart;

    MD5Context md5Context = _PckCreateMD5Context();

    for (unsigned i = 0; i < fileCount; i++) {
        PckBuildFile* file = files + i;

        char sizeStr[32];
        _PckFormatSize(file->hndl.size, sizeStr);

        printf("    - \"%s\" (%s)..", file->filename, sizeStr);

        PckString* name = (PckString*)nextEntry;
        name->size = sprintf(name->content, "res://%s", file->filename);

        nextEntry += sizeof(PckString) + _PckAlignUint(name->size);

        PckFileEntry* entry = (PckFileEntry*)nextEntry;
        entry->dataOffset = nextData - dataSectionStart;
        entry->dataSize = file->hndl.size;
        entry->flags = 0x00000000;

        _PckProduceMD5(
            file->hndl.data_void, file->hndl.size,
            md5Context,
            entry->md5Hash
        );

        nextEntry += sizeof(PckFileEntry);

        memcpy(nextData, file->hndl.data_void, file->hndl.size);
        nextData += _PckAlignUint(file->hndl.size);

        LOG_OK;
    }

    _PckEndMD5Context(md5Context);

    return resultHndl;
}
