#include "raylib.h"

#define RRES_IMPLEMENTATION
#include "rres.h"              // Required to read rres data chunks

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Load a continuous data buffer from rresResourceChunkData struct
static unsigned char *LoadDataBuffer(rresResourceChunkData data, unsigned int rawSize)
{
    unsigned char *buffer = (unsigned char *)RRES_CALLOC((data.propCount + 1)*sizeof(unsigned int) + rawSize, 1);
    
    memcpy(buffer, &data.propCount, sizeof(unsigned int));
    for (int i = 0; i < data.propCount; i++) memcpy(buffer + (i + 1)*sizeof(unsigned int), &data.props[i], sizeof(unsigned int));
    memcpy(buffer + (data.propCount + 1)*sizeof(unsigned int), data.raw, rawSize);
    
    return buffer;
}

// Unload data buffer
static void UnloadDataBuffer(unsigned char *buffer)
{
    RRES_FREE(buffer);
}

static void addTextFile(rresFileHeader* header, const char* filename, FILE* rresFile, rresDirEntry* entry)
{
    rresResourceChunkInfo chunkInfo = { 0 };    // Chunk info
    rresResourceChunkData chunkData = { 0 };    // Chunk data
                                                //
    unsigned char *buffer = NULL;

    // ----- Text files
    char *text = LoadFileText(filename);
    if (text == NULL)
    {
        printf("Cannot pack text file %s\n", filename);
        return;
    }
    unsigned int rawSize = strlen(text);

    // Define chunk info: TEXT
    chunkInfo.type[0] = 'T';         // Resource chunk type (FourCC)
    chunkInfo.type[1] = 'E';         // Resource chunk type (FourCC)
    chunkInfo.type[2] = 'X';         // Resource chunk type (FourCC)
    chunkInfo.type[3] = 'T';         // Resource chunk type (FourCC)
    
    // Resource chunk identifier (generated from filename CRC32 hash)
    chunkInfo.id = rresComputeCRC32((unsigned char*)filename, strlen(filename));

    if (entry != NULL)
    {
        entry->id = chunkInfo.id;
        entry->offset = ftell(rresFile);
        entry->fileNameSize = strlen(filename);
        strcpy(entry->fileName, filename);
    }
    
    chunkInfo.compType = RRES_COMP_NONE,     // Data compression algorithm
    chunkInfo.cipherType = RRES_CIPHER_NONE, // Data encription algorithm
    chunkInfo.flags = 0,             // Data flags (if required)
    chunkInfo.baseSize = 5*sizeof(unsigned int) + rawSize;   // Data base size (uncompressed/unencrypted)
    chunkInfo.packedSize = chunkInfo.baseSize; // Data chunk size (compressed/encrypted + custom data appended)
    chunkInfo.nextOffset = 0;        // Next resource chunk global offset (if resource has multiple chunks)
    chunkInfo.reserved = 0;          // <reserved>
                                     //
    // Define chunk data: TEXT
    chunkData.propCount = 4;
    chunkData.props = (unsigned int *)RRES_CALLOC(chunkData.propCount, sizeof(unsigned int));
    chunkData.props[0] = rawSize;    // props[0]:size (bytes)
    chunkData.props[1] = RRES_TEXT_ENCODING_UNDEFINED;  // props[1]:rresTextEncoding
    chunkData.props[2] = RRES_CODE_LANG_UNDEFINED;      // props[2]:rresCodeLang
    chunkData.props[3] = 0x0409;     // props[3]:cultureCode: en-US: English - United States
    chunkData.raw = text;
    // Get a continuous data buffer from chunkData
    buffer = LoadDataBuffer(chunkData, rawSize);
    
    // Compute data chunk CRC32 (propCount + props[] + data)
    chunkInfo.crc32 = rresComputeCRC32(buffer, chunkInfo.packedSize);

    // Write resource chunk into rres file
    fwrite(&chunkInfo, sizeof(rresResourceChunkInfo), 1, rresFile);
    fwrite(buffer, 1, chunkInfo.packedSize, rresFile);
    
    // Free required memory
    RRES_FREE(chunkData.props);
    UnloadDataBuffer(buffer);
    UnloadFileText(text);

    header->chunkCount++;
}

static bool addRawData(rresFileHeader* header, const char* filename, FILE* rresFile, const char* ext, rresDirEntry* entry)
{
    rresResourceChunkInfo chunkInfo = { 0 };    // Chunk info
    rresResourceChunkData chunkData = { 0 };    // Chunk data
    unsigned char *buffer = NULL;

    // Load file data
    unsigned int size = 0;
    unsigned char* raw = LoadFileData(filename, &size);
    if (raw == NULL)
    {
        printf("Cannot pack raw file %s\n", filename);
        UnloadDataBuffer(buffer);
        return false;
    }

    // Define chunk info: RAWD
    chunkInfo.type[0] = 'R';         // Resource chunk type (FourCC)
    chunkInfo.type[1] = 'A';         // Resource chunk type (FourCC)
    chunkInfo.type[2] = 'W';         // Resource chunk type (FourCC)
    chunkInfo.type[3] = 'D';         // Resource chunk type (FourCC)
    
    // Resource chunk identifier (generated from filename CRC32 hash)
    chunkInfo.id = rresComputeCRC32((unsigned char*)filename, strlen(filename));
    
    if (entry != NULL)
    {
        entry->id = chunkInfo.id;
        entry->offset = ftell(rresFile);
        entry->fileNameSize = strlen(filename);
        strcpy(entry->fileName, filename);
    }

    chunkInfo.compType = RRES_COMP_NONE,     // Data compression algorithm
    chunkInfo.cipherType = RRES_CIPHER_NONE, // Data encription algorithm
    chunkInfo.flags = 0,             // Data flags (if required)
    chunkInfo.baseSize = 5*sizeof(unsigned int) + size;   // Data base size (uncompressed/unencrypted)
    chunkInfo.packedSize = chunkInfo.baseSize; // Data chunk size (compressed/encrypted + custom data appended)
    chunkInfo.nextOffset = 0,        // Next resource chunk global offset (if resource has multiple chunks)
    chunkInfo.reserved = 0,          // <reserved>
    
    // Define chunk data: IMGE
    chunkData.propCount = 4;
    chunkData.props = (unsigned int *)RRES_CALLOC(chunkData.propCount, sizeof(unsigned int));
    chunkData.props[0] = size;      // props[0]:width
    memcpy(chunkData.props + 1, ext, 4);
    chunkData.props[2] = 0x0;     // props[2]:rresPixelFormat
    chunkData.props[3] = 0x0;     // props[2]:rresPixelFormat
    chunkData.raw = raw;
    

    // Get a continuous data buffer from chunkData
    buffer = LoadDataBuffer(chunkData, size);
    
    // Compute data chunk CRC32 (propCount + props[] + data)
    chunkInfo.crc32 = rresComputeCRC32(buffer, chunkInfo.packedSize);

    // Write resource chunk into rres file
    fwrite(&chunkInfo, sizeof(rresResourceChunkInfo), 1, rresFile);
    fwrite(buffer, 1, chunkInfo.packedSize, rresFile);
    
    // Free required memory
    RRES_FREE(chunkData.props);
    UnloadDataBuffer(buffer);
    UnloadFileData(raw);

    header->chunkCount++;
    return true;
}

static void addCentralDir(rresFileHeader* header, const rresCentralDir* central, FILE* rresFile)
{
    rresResourceChunkInfo chunkInfo = { 0 };    // Chunk info
    unsigned char *buffer = NULL;

    if (header->cdOffset != 0)
    {
        puts("There's is already a Central Dir");
        return;
    }
    header->cdOffset = ftell(rresFile) - sizeof(rresFileHeader);

    unsigned int size = 0;
    // TODO: Compute correctly the size
    for (unsigned int i = 0; i < central->count; ++i)
    {
        size += 4 * sizeof(unsigned int) + central->entries[i].fileNameSize;
    }
    printf("Cdir size: %d\n", size);

    unsigned int props[2] = {1, central->count};

    // Define chunk info: CDIR
    chunkInfo.type[0] = 'C';         // Resource chunk type (FourCC)
    chunkInfo.type[1] = 'D';         // Resource chunk type (FourCC)
    chunkInfo.type[2] = 'I';         // Resource chunk type (FourCC)
    chunkInfo.type[3] = 'R';         // Resource chunk type (FourCC)


    chunkInfo.id = 0xCD010203;
    chunkInfo.compType = RRES_COMP_NONE,     // Data compression algorithm
    chunkInfo.cipherType = RRES_CIPHER_NONE, // Data encription algorithm
    chunkInfo.flags = 0,             // Data flags (if required)
    chunkInfo.baseSize = sizeof(props) + size;   // Data base size (uncompressed/unencrypted)
    chunkInfo.packedSize = chunkInfo.baseSize; // Data chunk size (compressed/encrypted + custom data appended)
    chunkInfo.nextOffset = 0;        // Next resource chunk global offset (if resource has multiple chunks)
    chunkInfo.reserved = 0;          // <reserved>

    buffer = (unsigned char *)RRES_CALLOC(sizeof(props) + size, 1);
    memcpy(buffer, props, sizeof(props));

    unsigned char* data_ptr = buffer + sizeof(props);
    for (unsigned int i = 0; i < central->count; ++i)
    {
        printf("Recording CDIR for %s\n", central->entries[i].fileName);
        unsigned int n_write = 4 * sizeof(unsigned int) + central->entries[i].fileNameSize;
        memcpy(data_ptr, central->entries + i, n_write);
        data_ptr += n_write;
    }

    // Compute data chunk CRC32 (propCount + props[] + data)
    chunkInfo.crc32 = rresComputeCRC32(buffer, chunkInfo.packedSize);

    // Write resource chunk into rres file
    fwrite(&chunkInfo, sizeof(rresResourceChunkInfo), 1, rresFile);
    fwrite(buffer, 1, chunkInfo.packedSize, rresFile);
    
    // Free required memory
    UnloadDataBuffer(buffer);
    header->chunkCount++;
}

typedef enum AssetInfoType
{
    TEXTURE_INFO,
    SPRITE_INFO,
    LEVELPACK_INFO,
    SOUND_INFO,
    INVALID_INFO
}AssetInfoType_t;

int main(void)
{
    FILE *rresFile = fopen("myresources.rres", "wb");

    // Define rres file header
    // NOTE: We are loading 4 files that generate 5 resource chunks to save in rres
    rresFileHeader header = {
        .id[0] = 'r',           // File identifier: rres
        .id[1] = 'r',           // File identifier: rres
        .id[2] = 'e',           // File identifier: rres
        .id[3] = 's',           // File identifier: rres
        .version = 100,         // File version: 100 for version 1.0
        .chunkCount = 0,        // Number of resource chunks in the file (MAX: 65535)
        .cdOffset = 0,          // Central Directory offset in file (0 if not available)
        .reserved = 0           // <reserved>
    };
    
#define STARTING_CHUNKS 64
    // Central Directory
    rresCentralDir central = {
        .count = 0,
        .entries = NULL
    };
    central.entries = RRES_CALLOC(sizeof(rresDirEntry), STARTING_CHUNKS);
    fseek(rresFile, sizeof(rresFileHeader), SEEK_SET);
    

    addTextFile(&header, "assets.info", rresFile, central.entries);
    addTextFile(&header, "player_spr.info", rresFile, central.entries + 1);
    central.count = 2;
    uint16_t max_chunks = STARTING_CHUNKS;

    {
        FILE* in_file = fopen("assets.info", "r");
        if (in_file == NULL)
        {
            puts("assets.info must be present");
            goto end;
        }
        
        char buffer[256];
        char* tmp;
        AssetInfoType_t info_type = INVALID_INFO;

        while (true)
        {
            tmp = fgets(buffer, 256, in_file);
            if (tmp == NULL) break;
            tmp[strcspn(tmp, "\r\n")] = '\0';

            if (central.count == max_chunks)
            {
                void* new_ptr = realloc(central.entries, (max_chunks*2) * sizeof(rresDirEntry));
                if (new_ptr == NULL)
                {
                    puts("Cannot realloc central entries");
                    fclose(in_file);
                    goto end;
                }
                central.entries = new_ptr;
                max_chunks *= 2;
            }

            if (tmp[0] == '-')
            {
                tmp++;
                if (strcmp(tmp, "Texture") == 0)
                {
                    info_type = TEXTURE_INFO;
                }
                else if (strcmp(tmp, "LevelPack") == 0)
                {
                    info_type = LEVELPACK_INFO;
                }
                else if (strcmp(tmp, "Sound") == 0)
                {
                    info_type = SOUND_INFO;
                }
                else
                {
                    info_type = INVALID_INFO;
                }
            }
            else
            {
                char* name = strtok(buffer, ":");
                char* info_str = strtok(NULL, ":");
                if (name == NULL || info_str == NULL) continue;

                while(*name == ' ' || *name == '\t') name++;
                while(*info_str == ' ' || *info_str == '\t') info_str++;
                switch(info_type)
                {
                    case TEXTURE_INFO:
                    {
                        // ---- SpriteSheets
                        if (
                            addRawData(
                                &header, info_str,
                                rresFile, GetFileExtension(info_str),
                                central.entries + central.count
                           )
                        )
                        {
                            central.count++;
                        }
                    }
                    break;
                    case SOUND_INFO:
                    {
                        // ---- OGG Sound
                        if (
                            addRawData(
                                &header, info_str,
                                rresFile, ".ogg",
                                central.entries + central.count
                            )
                        )
                        {
                            central.count++;
                        }
                    }
                    break;
                    case LEVELPACK_INFO:
                    {
                        // ---- Compressed Level Data
                        if (
                            addRawData(
                                &header, info_str,
                                rresFile, ".lpk",
                                central.entries + central.count
                            )
                        )
                        {
                            central.count++;
                        }
                    }
                    break;
                    default:
                    break;
                }
            }
        }
        fclose(in_file);
    }

    addCentralDir(&header, &central, rresFile); 

    // Write rres file header
    fseek(rresFile, 0, SEEK_SET);
    fwrite(&header, sizeof(rresFileHeader), 1, rresFile);

end:
    fclose(rresFile);
    RRES_FREE(central.entries);
    return 0;
}
