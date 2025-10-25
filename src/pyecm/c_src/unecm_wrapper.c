#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ecm_tools.h"

#define ecc_uint8 unsigned char
#define ecc_uint16 unsigned short
#define ecc_uint32 unsigned

static ecc_uint8 unecm_ecc_f_lut[256];
static ecc_uint8 unecm_ecc_b_lut[256];
static ecc_uint32 unecm_edc_lut[256];

void unecm_eccedc_init(void) {
  ecc_uint32 i, j, edc;
  for(i = 0; i < 256; i++) {
    j = (i << 1) ^ (i & 0x80 ? 0x11D : 0);
    unecm_ecc_f_lut[i] = j;
    unecm_ecc_b_lut[i ^ j] = i;
    edc = i;
    for(j = 0; j < 8; j++) edc = (edc >> 1) ^ (edc & 1 ? 0xD8018001 : 0);
    unecm_edc_lut[i] = edc;
  }
}

static ecc_uint32 unecm_edc_partial_computeblock(ecc_uint32 edc, const ecc_uint8 *src, ecc_uint16 size) {
  while(size--) edc = (edc >> 8) ^ unecm_edc_lut[(edc ^ (*src++)) & 0xFF];
  return edc;
}

static void unecm_edc_computeblock(const ecc_uint8 *src, ecc_uint16 size, ecc_uint8 *dest) {
  ecc_uint32 edc = unecm_edc_partial_computeblock(0, src, size);
  dest[0] = (edc >> 0) & 0xFF;
  dest[1] = (edc >> 8) & 0xFF;
  dest[2] = (edc >> 16) & 0xFF;
  dest[3] = (edc >> 24) & 0xFF;
}

static void unecm_ecc_computeblock(ecc_uint8 *src, ecc_uint32 major_count, ecc_uint32 minor_count, ecc_uint32 major_mult, ecc_uint32 minor_inc, ecc_uint8 *dest) {
  ecc_uint32 size = major_count * minor_count;
  ecc_uint32 major, minor;
  for(major = 0; major < major_count; major++) {
    ecc_uint32 index = (major >> 1) * major_mult + (major & 1);
    ecc_uint8 ecc_a = 0;
    ecc_uint8 ecc_b = 0;
    for(minor = 0; minor < minor_count; minor++) {
      ecc_uint8 temp = src[index];
      index += minor_inc;
      if(index >= size) index -= size;
      ecc_a ^= temp;
      ecc_b ^= temp;
      ecc_a = unecm_ecc_f_lut[ecc_a];
    }
    ecc_a = unecm_ecc_b_lut[unecm_ecc_f_lut[ecc_a] ^ ecc_b];
    dest[major] = ecc_a;
    dest[major + major_count] = ecc_a ^ ecc_b;
  }
}

static void unecm_ecc_generate(ecc_uint8 *sector, int zeroaddress) {
  ecc_uint8 address[4], i;
  if(zeroaddress) for(i = 0; i < 4; i++) {
    address[i] = sector[12 + i];
    sector[12 + i] = 0;
  }
  unecm_ecc_computeblock(sector + 0xC, 86, 24, 2, 86, sector + 0x81C);
  unecm_ecc_computeblock(sector + 0xC, 52, 43, 86, 88, sector + 0x8C8);
  if(zeroaddress) for(i = 0; i < 4; i++) sector[12 + i] = address[i];
}

static void unecm_eccedc_generate(ecc_uint8 *sector, int type) {
  ecc_uint32 i;
  switch(type) {
  case 1:
    unecm_edc_computeblock(sector + 0x00, 0x810, sector + 0x810);
    for(i = 0; i < 8; i++) sector[0x814 + i] = 0;
    unecm_ecc_generate(sector, 0);
    break;
  case 2:
    unecm_edc_computeblock(sector + 0x10, 0x808, sector + 0x818);
    unecm_ecc_generate(sector, 1);
    break;
  case 3:
    unecm_edc_computeblock(sector + 0x10, 0x91C, sector + 0x92C);
    break;
  }
}

static unsigned unecm_mycounter;
static unsigned unecm_mycounter_total;
static progress_callback unecm_progress_cb = NULL;

static void unecm_resetcounter(unsigned total, progress_callback callback) {
  unecm_mycounter = 0;
  unecm_mycounter_total = total;
  unecm_progress_cb = callback;
}

static void unecm_setcounter(unsigned n) {
  if((n >> 20) != (unecm_mycounter >> 20)) {
    if (unecm_progress_cb) unecm_progress_cb(n, unecm_mycounter_total, 1);
  }
  unecm_mycounter = n;
}

int unecm_unecmify(FILE *infile, FILE *outfile, progress_callback callback) {
  unsigned checkedc = 0;
  unsigned char sector[2352];
  unsigned type;
  unsigned num;

  fseek(infile, 0, SEEK_END);
  unecm_resetcounter(ftell(infile), callback);
  fseek(infile, 0, SEEK_SET);

  if((fgetc(infile)!='E')||(fgetc(infile)!='C')||(fgetc(infile)!='M')||(fgetc(infile)!=0x00)) {
    return 1; // Header not found
  }

  for(;;) {
    int c = fgetc(infile);
    int bits = 5;
    if(c == EOF) return 1; // Unexpected EOF
    type = c & 3;
    num = (c >> 2) & 0x1F;
    while(c & 0x80) {
      c = fgetc(infile);
      if(c == EOF) return 1; // Unexpected EOF
      num |= ((unsigned)(c & 0x7F)) << bits;
      bits += 7;
    }
    if(num == 0xFFFFFFFF) break;
    num++;
    if(num >= 0x80000000) return 1; // Corrupt

    if(!type) {
      while(num) {
        int b = num;
        if(b > 2352) b = 2352;
        if(fread(sector, 1, b, infile) != b) return 1; // Unexpected EOF
        checkedc = unecm_edc_partial_computeblock(checkedc, sector, b);
        fwrite(sector, 1, b, outfile);
        num -= b;
        unecm_setcounter(ftell(infile));
      }
    } else {
      while(num--) {
        memset(sector, 0, sizeof(sector));
        memset(sector + 1, 0xFF, 10);
        switch(type) {
        case 1:
          sector[0x0F] = 0x01;
          if(fread(sector + 0x00C, 1, 0x003, infile) != 0x003) return 1;
          if(fread(sector + 0x010, 1, 0x800, infile) != 0x800) return 1;
          unecm_eccedc_generate(sector, 1);
          checkedc = unecm_edc_partial_computeblock(checkedc, sector, 2352);
          fwrite(sector, 2352, 1, outfile);
          unecm_setcounter(ftell(infile));
          break;
        case 2:
          sector[0x0F] = 0x02;
          if(fread(sector + 0x014, 1, 0x804, infile) != 0x804) return 1;
          sector[0x10] = sector[0x14]; sector[0x11] = sector[0x15];
          sector[0x12] = sector[0x16]; sector[0x13] = sector[0x17];
          unecm_eccedc_generate(sector, 2);
          checkedc = unecm_edc_partial_computeblock(checkedc, sector + 0x10, 2336);
          fwrite(sector + 0x10, 2336, 1, outfile);
          unecm_setcounter(ftell(infile));
          break;
        case 3:
          sector[0x0F] = 0x02;
          if(fread(sector + 0x014, 1, 0x918, infile) != 0x918) return 1;
          sector[0x10] = sector[0x14]; sector[0x11] = sector[0x15];
          sector[0x12] = sector[0x16]; sector[0x13] = sector[0x17];
          unecm_eccedc_generate(sector, 3);
          checkedc = unecm_edc_partial_computeblock(checkedc, sector + 0x10, 2336);
          fwrite(sector + 0x10, 2336, 1, outfile);
          unecm_setcounter(ftell(infile));
          break;
        }
      }
    }
  }

  if(fread(sector, 1, 4, infile) != 4) return 1; // Unexpected EOF

  if((sector[0]!=((checkedc>>0)&0xFF))||(sector[1]!=((checkedc>>8)&0xFF))||(sector[2]!=((checkedc>>16)&0xFF))||(sector[3]!=((checkedc>>24)&0xFF))) {
    return 1; // EDC error
  }

  // Final progress update to show 100%
  if (unecm_progress_cb) unecm_progress_cb(unecm_mycounter_total, unecm_mycounter_total, 1);

  return 0; // Success
}