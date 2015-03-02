#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "cipher.h"

// how big to split decryption steps
#define WRITE_BUFFER 1024
// brackets ho!
#define ROTL(val, bits) (((val) << (bits)) | ((val) >> (32-(bits))))

// No comments 4 lyf

typedef struct {
    FILE* stream;
    unsigned int decryptedSize;
    int* cipherKey;
    int* fileKey;
    int keySize;
} CryptFile;


void decrypt(CryptFile* bigFile, char *destStr, int size, unsigned int keyOffset)
{
  int i;
  
  fread(destStr, size, 1u, bigFile->stream);

  for(i = 0; i < size; i++) {
    destStr[i] += *((char *)bigFile->cipherKey + (i + keyOffset) % bigFile->keySize);
  }
}

int decrypt_all(CryptFile* bigFile, char* destFile) {
    // 4GB MAX, MAY BE EXCEEDED AT SOME POINT
    unsigned int chunkBytes;
    char buffer[WRITE_BUFFER];
    unsigned int current = 0;
    FILE* dest = fopen(destFile, "wb");
    
    if(!dest) {
        printf("Failed to open output file %s\n", destFile);
        return 1;
    }
    
    while(current < bigFile->decryptedSize) {
        if(current + WRITE_BUFFER > bigFile->decryptedSize) {
            chunkBytes = bigFile->decryptedSize - current;
        } else {
            chunkBytes = WRITE_BUFFER;
        }
        decrypt(bigFile, buffer, chunkBytes, current);
        fwrite(buffer, chunkBytes, 1u, dest);
        current += chunkBytes;
    }
    fclose(dest);
    return 0;
}

// Decompiled, cleaned up a ton, still not entirely sure how crazy xor ciphers work.
void cipher_magic(CryptFile* bigFile) {
  int i, j;
  int byte;
  unsigned int currentKey;
  unsigned int tempVal;
  unsigned char *tempBytes;

  printf("Decrypting cipher...\n");
  if ( bigFile->keySize > 0 )
  {
    for(i = 0; i < bigFile->keySize; i+= 4) {
      currentKey = bigFile->fileKey[i / 4];
      for(byte = 0; byte < 4; byte++) {         
        tempVal = ROTL(currentKey + bigFile->decryptedSize, 8);
        tempBytes = (uint8_t*)&tempVal;
        for(j = 0; j < 4; j++) {
          currentKey = cipherConst[(uint8_t)(currentKey ^ tempBytes[j])] ^ (currentKey >> 8);
        }
        *((char *)bigFile->cipherKey + byte + i) = (char) currentKey;
      }
    }
  }
}

int loadCipher(CryptFile* bigFile)
{
  int temp;
  int lastIntLoc;

  temp = 0;
  fseek(bigFile->stream, -4, SEEK_END);
  lastIntLoc = ftell(bigFile->stream);
  fread(&temp, 4u, 1u, bigFile->stream); // how far back do we need to go?
  printf("0xDEADBE7A should be %d bytes from file end\n", temp);
  if ( temp )
  {
    if ( temp < (unsigned int)(lastIntLoc - 6) )
    {
      fseek(bigFile->stream, -temp, SEEK_CUR);
      bigFile->decryptedSize = ftell(bigFile->stream);
      temp = 0;
      fread(&temp, 4u, 1u, bigFile->stream);
      if ( temp == 0xDEADBE7A  ) {
        printf("Found 0xDEADBE7A, located at byte %d\n", bigFile->decryptedSize);
        temp = 0;
        fread(&temp, 2u, 1u, bigFile->stream);
        bigFile->keySize = temp;
        if ( (unsigned int)(temp - 1) <= 0x3FF ) {
          printf("Keysize detected: %d\n", bigFile->keySize);
          bigFile->cipherKey = (int*) malloc(sizeof(char) * bigFile->keySize);
          bigFile->fileKey   = (int*) malloc(sizeof(char) * bigFile->keySize);
          fread((void *)(bigFile->fileKey), bigFile->keySize, 1u, bigFile->stream);
          cipher_magic(bigFile);
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
  fseek(bigFile->stream, 0, 0);
  return 0;
}

int main(int argc, char** argv) {
    CryptFile bigFile;
    
    if(argc != 3) {
        printf("Homeworld Remastered .big decrypter by monty. http://github.com/mon\n");
        printf("  Usage: bigDecrypter.exe encryptedBig outputBig\n");
        return 1;
    }
    
    printf("Opening %s...\n", argv[1]);
    if(!(bigFile.stream = fopen(argv[1], "rb"))) {
        printf("Failed to open input file %s\n", argv[1]);
        return 1;
    }
    
    printf("Loading cipher...\n");
    if(loadCipher(&bigFile)) {
        return 1;
    }
    printf("Cipher loaded!\n");
    
    printf("Decrypting %ukb from %s into %s...\n", bigFile.decryptedSize / 1024, argv[1], argv[2]);
    if(decrypt_all(&bigFile, argv[2])) {
        return 1;
    }
    fclose(bigFile.stream);
    printf("\nDecrypt success!\n");
    return 0;
}

