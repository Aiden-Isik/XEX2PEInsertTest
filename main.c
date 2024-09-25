// XEX2 PE Insertion Test
// Copyright (c) 2024 Aiden Isik
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <arpa/inet.h>

#if 'AB' == 0b100000101000010
#define LITTLE_ENDIAN_SYSTEM
#else
#define BIG_ENDIAN_SYSTEM
#endif

#define LITTLE_ENDIAN_FILE false
#define BIG_ENDIAN_FILE true

// These structs should be converted to BIG ENDIAN immediately before being written out
struct __attribute__((packed)) dataDescriptor
{
  uint32_t size;
  uint16_t encryption;
  uint16_t compression;
};

struct __attribute__((packed)) rawDataDescriptor
{
  uint32_t dataSize;
  uint32_t zeroSize;
};

uint32_t get32BitFromFile(FILE *file, bool endianness)
{
  uint8_t store[4];
  fread(store, sizeof(uint8_t), 4, file);
  
  uint32_t result = 0;

  for(int i = 0; i < 4; i++)
    {
      result |= store[i] << i * 8;
    }

  // If system and file endianness don't match we need to change it
#ifdef LITTLE_ENDIAN_SYSTEM
  if(endianness != LITTLE_ENDIAN_FILE)
#else
  if(endianness != BIG_ENDIAN_FILE)
#endif
    {
      switch(endianness)
	{
	case LITTLE_ENDIAN_FILE:
	  result = htonl(result);
	  break;
	case BIG_ENDIAN_FILE:
	  result = ntohl(result);
	  break;
	}
    }

  return result;
}

uint32_t get16BitFromFile(FILE *file, bool endianness)
{
  uint8_t store[2];
  fread(store, sizeof(uint8_t), 2, file);
  
  uint32_t result = 0;

  for(int i = 0; i < 2; i++)
    {
      result |= store[i] << i * 8;
    }

  // If system and file endianness don't match we need to change it
#ifdef LITTLE_ENDIAN_SYSTEM
  if(endianness != LITTLE_ENDIAN_FILE)
#else
  if(endianness != BIG_ENDIAN_FILE)
#endif
    {
      switch(endianness)
	{
	case LITTLE_ENDIAN_FILE:
	  result = htons(result);
	  break;
	case BIG_ENDIAN_FILE:
	  result = ntohs(result);
	  break;
	}
    }

  return result;
}

int main(int argc, char **argv)
{
  FILE *xex = fopen(argv[2], "w+");
  FILE *pe = fopen(argv[1], "r");
  
  struct dataDescriptor dataDesc;
  dataDesc.size = (1 * 8) + 8; // (Block count * size of raw data descriptor) + size of data descriptor
  dataDesc.encryption = 0x0; // No encryption
  dataDesc.compression = 0x1; // "Raw", no compression

  struct rawDataDescriptor rawDataDesc;
  fseek(pe, 0, SEEK_END);
  rawDataDesc.dataSize = ftell(pe);
  rawDataDesc.zeroSize = 0x0; // We aren't going to be removing any zeroes. TODO: implement this, it can make files much smaller

  uint32_t basefileOffset = 0x0 + dataDesc.size;
  uint32_t dataDescOffset = 0x0;

  // Copy PE basefile to XEX (before struct endianness is converted, we need those values!)
  fseek(xex, basefileOffset, SEEK_SET);
  fseek(pe, 0, SEEK_SET);

  uint16_t readBufSize = 0x1000; // Reading in 4KiB at a time to avoid excessive memory usage
  uint8_t *buffer = malloc(readBufSize * sizeof(uint8_t));
  
  for(uint32_t i = 0; i < rawDataDesc.dataSize; i += readBufSize)
    {
      fread(buffer, sizeof(uint8_t), readBufSize, pe);
      fwrite(buffer, sizeof(uint8_t), readBufSize, xex);
    }

  free(buffer);
  
  // Set structs to big endian before writing
#ifdef LITTLE_ENDIAN_SYSTEM
  dataDesc.size = htonl(dataDesc.size);
  dataDesc.encryption = htons(dataDesc.encryption);
  dataDesc.compression = htons(dataDesc.compression);

  rawDataDesc.dataSize = htonl(rawDataDesc.dataSize);
  rawDataDesc.zeroSize = htonl(rawDataDesc.zeroSize);
#endif

  // Write data descriptor information to XEX
  fseek(xex, dataDescOffset, SEEK_SET);
  fwrite(&dataDesc, sizeof(dataDesc), 1, xex);
  fwrite(&rawDataDesc, sizeof(rawDataDesc), 1, xex);

  fclose(pe);
  fclose(xex);
  return 0;
}
