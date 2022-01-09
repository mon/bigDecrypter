#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cipher.h"

#define VERSION "1.5"

/* how big to split decryption steps */
#define WRITE_BUFFER_SIZE (1024 * 1024 * 8)
/* 80 chars, make it wider if you just love progress */
#define PROGRESS_WIDTH 80
/* many brackets */
#define ROTL(val, bits) (((val) << (bits)) | ((val) >> (32-(bits))))

typedef struct {
    FILE* stream;
    long decryptedSize;
    long tocSize;
    long bonusData;
    long bonusDataSize;
    uint32_t* cipherKey;
    uint32_t* fileKey;
    uint16_t keySize;
} CryptFile;

void update_progress(float percent) {
    int i;
    static float last = -1;
    const int actualWidth = PROGRESS_WIDTH - 2;

    /* Don't update if there is nothing to update */
    if(percent - last <  1.0 / actualWidth)
        return;
    printf("\r[");
    for(i = 0; i < actualWidth; i++) {
        if( (float)i / (float)actualWidth < percent) {
            printf("=");
        } else {
            printf(" ");
        }
    }
    printf("]");
    fflush(stdout);
    last = percent;
}

void decrypt(CryptFile* bigFile, char *destStr, size_t size, uint32_t keyOffset)
{
  size_t i;

  fread(destStr, size, 1u, bigFile->stream);

  for(i = 0; i < size; i++) {
    destStr[i] += *((char *)bigFile->cipherKey + (i + keyOffset) % bigFile->keySize);
  }
}

int decrypt_all(CryptFile* bigFile, char* destFile) {
    /* 4GB MAX, MAY BE EXCEEDED AT SOME POINT */
    long chunkBytes;
    long current = 0;
    char *buffer = (char*) malloc(WRITE_BUFFER_SIZE);
    FILE *dest = fopen(destFile, "wb");

    if(!buffer) {
        printf("Couldn't allocate decryption buffer. No RAM?\n");
        return 1;
    }
    if(!dest) {
        printf("Failed to open output file %s\n", destFile);
        return 1;
    }

    while(current < bigFile->decryptedSize) {
        if(current + WRITE_BUFFER_SIZE > bigFile->decryptedSize) {
            chunkBytes = bigFile->decryptedSize - current;
        } else {
            chunkBytes = WRITE_BUFFER_SIZE;
        }
        decrypt(bigFile, buffer, chunkBytes, current);
        fwrite(buffer, chunkBytes, 1u, dest);
        current += chunkBytes;
        update_progress((float)current / (float)bigFile->decryptedSize);
    }
    fclose(dest);
    return 0;
}

/* Decompiled, cleaned up a ton, still not entirely sure how crazy xor ciphers work. */
void cipher_magic(CryptFile* bigFile) {
  uint32_t i, j;
  int byte;
  uint32_t currentKey;
  uint32_t tempVal;
  uint8_t *tempBytes;

  printf("Decrypting cipher...\n");
  for(i = 0; i < bigFile->keySize; i+= 4) {
    currentKey = bigFile->fileKey[i / 4];
    for(byte = 0; byte < 4; byte++) {
      tempVal = ROTL(currentKey + bigFile->tocSize, 8);
      tempBytes = (uint8_t*)&tempVal;
      for(j = 0; j < 4; j++) {
        currentKey = cipherConst[(uint8_t)(currentKey ^ tempBytes[j])] ^ (currentKey >> 8);
      }
      *((uint8_t *)bigFile->cipherKey + byte + i) = (uint8_t) currentKey;
    }
}
}

int loadCipher(CryptFile* bigFile)
{
  uint32_t sentinelLoc;
  uint32_t deadbe7a;
  uint32_t lastIntLoc;

  fseek(bigFile->stream, -4, SEEK_END);
  lastIntLoc = ftell(bigFile->stream);
  fread(&sentinelLoc, 4u, 1u, bigFile->stream); /* how far back do we need to go? */
  printf("0xDEADBE7A should be %d bytes from file end\n", sentinelLoc);
  if ( sentinelLoc )
  {
    if ( sentinelLoc < lastIntLoc - 6)
    {
      fseek(bigFile->stream, -(long)sentinelLoc, SEEK_CUR);
      bigFile->tocSize = ftell(bigFile->stream);
      fread(&deadbe7a, 4u, 1u, bigFile->stream);
      if ( deadbe7a == 0xDEADBE7A  ) {
        printf("Found 0xDEADBE7A, located at byte %ld\n", bigFile->tocSize);
        fread(&bigFile->keySize, 2u, 1u, bigFile->stream);
        if ( bigFile->keySize - 1 <= 0x3FF ) {
          printf("Keysize detected: %d\n", bigFile->keySize);
          bigFile->cipherKey = (uint32_t*) malloc(sizeof(char) * bigFile->keySize);
          bigFile->fileKey   = (uint32_t*) malloc(sizeof(char) * bigFile->keySize);
          fread((void *)(bigFile->fileKey), bigFile->keySize, 1u, bigFile->stream);
          cipher_magic(bigFile);

          bigFile->bonusData = ftell(bigFile->stream);
          bigFile->bonusDataSize = lastIntLoc - bigFile->bonusData;
          if(bigFile->bonusDataSize) {
              printf("Found %ldkb of bonus data\n", bigFile->bonusDataSize / 1024);
              bigFile->decryptedSize = lastIntLoc;
          } else {
              bigFile->decryptedSize = bigFile->tocSize;
          }

        } else {
            printf("Keysize too big, 0x%x > 0x3FF\n", bigFile->keySize);
            return 1;
        }
      } else {
          printf("Could not find 0xDEADBE7A, is file wrong format or not encrypted?\n");
          return 1;
      }
    } else {
        printf("Encryption block location is not in file, is file wrong format or not encrypted?\n");
        return 1;
    }
  } else {
      printf("Could not read encryption block location, is file wrong format or not encrypted?\n");
      return 1;
  }
  fseek(bigFile->stream, 0, SEEK_SET);
  return 0;
}

int main(int argc, char** argv) {
    CryptFile bigFile;
    char archiveStr[9];

    printf("Homeworld Remastered .big decrypter v%s by monty. http://github.com/mon/bigDecrypter\n", VERSION);

    if(argc != 3) {
        printf("  Usage: bigDecrypter.exe encryptedBig.big outputBig.big\n");
        return 1;
    }

    if(!strcmp(argv[1], argv[2])) {
        printf("Input filename equals output filename. You don't want this. Exiting...\n");
        return 1;
    }
    printf("Opening %s...\n", argv[1]);
    if(!(bigFile.stream = fopen(argv[1], "rb"))) {
        printf("Failed to open input file %s. Are you in the Homeworld Data directory?\n", argv[1]);
        return 1;
    }

    printf("Performing sanity check...\n");
    fgets(archiveStr, 9, bigFile.stream);
    if(!strcmp(archiveStr, "_ARCHIVE")) {
        printf("File begins with _ARCHIVE, This file is NOT encrypted! No need to run this.\n");
        return 1;
    }
    fseek(bigFile.stream, 0, SEEK_SET);

    printf("Loading cipher...\n");
    if(loadCipher(&bigFile)) {
        return 1;
    }
    printf("Cipher loaded!\n");

    printf("Decrypting %ldkb from %s into %s...\n", bigFile.decryptedSize / 1024, argv[1], argv[2]);
    if(decrypt_all(&bigFile, argv[2])) {
        return 1;
    }
    fclose(bigFile.stream);
    printf("\n\nDecrypt success!\n");
    return 0;
}

