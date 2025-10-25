#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ecm_tools.h"

#define ecc_uint8 unsigned char
#define ecc_uint16 unsigned short
#define ecc_uint32 unsigned

static ecc_uint8 ecm_ecc_f_lut[256];
static ecc_uint8 ecm_ecc_b_lut[256];
static ecc_uint32 ecm_edc_lut[256];

void ecm_eccedc_init(void) {
  ecc_uint32 i, j, edc;
  for(i = 0; i < 256; i++) {
    j = (i << 1) ^ (i & 0x80 ? 0x11D : 0);
    ecm_ecc_f_lut[i] = j;
    ecm_ecc_b_lut[i ^ j] = i;
    edc = i;
    for(j = 0; j < 8; j++) edc = (edc >> 1) ^ (edc & 1 ? 0xD8018001 : 0);
    ecm_edc_lut[i] = edc;
  }
}

static ecc_uint32 ecm_edc_computeblock(ecc_uint32 edc, const ecc_uint8 *src, ecc_uint16 size) {
  while(size--) edc = (edc >> 8) ^ ecm_edc_lut[(edc ^ (*src++)) & 0xFF];
  return edc;
}

static int ecm_ecc_computeblock(ecc_uint8 *src, ecc_uint32 major_count, ecc_uint32 minor_count, ecc_uint32 major_mult, ecc_uint32 minor_inc, ecc_uint8 *dest) {
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
      ecc_a = ecm_ecc_f_lut[ecc_a];
    }
    ecc_a = ecm_ecc_b_lut[ecm_ecc_f_lut[ecc_a] ^ ecc_b];
    if(dest[major] != ecc_a) return 0;
    if(dest[major + major_count] != (ecc_a ^ ecc_b)) return 0;
  }
  return 1;
}

static int ecm_ecc_generate(ecc_uint8 *sector, int zeroaddress, ecc_uint8 *dest) {
  int r;
  ecc_uint8 address[4], i;
  if(zeroaddress) for(i = 0; i < 4; i++) {
    address[i] = sector[12 + i];
    sector[12 + i] = 0;
  }
  if(!(ecm_ecc_computeblock(sector + 0xC, 86, 24,  2, 86, dest + 0x81C - 0x81C))) {
    if(zeroaddress) for(i = 0; i < 4; i++) sector[12 + i] = address[i];
    return 0;
  }
  r = ecm_ecc_computeblock(sector + 0xC, 52, 43, 86, 88, dest + 0x8C8 - 0x81C);
  if(zeroaddress) for(i = 0; i < 4; i++) sector[12 + i] = address[i];
  return r;
}

static int ecm_check_type(unsigned char *sector, int canbetype1) {
  int canbetype2 = 1;
  int canbetype3 = 1;
  ecc_uint32 myedc;
  if(canbetype1) {
    if((sector[0x00]!=0x00)||(sector[0x01]!=0xFF)||(sector[0x02]!=0xFF)||(sector[0x03]!=0xFF)||(sector[0x04]!=0xFF)||(sector[0x05]!=0xFF)||(sector[0x06]!=0xFF)||(sector[0x07]!=0xFF)||(sector[0x08]!=0xFF)||(sector[0x09]!=0xFF)||(sector[0x0A]!=0xFF)||(sector[0x0B]!=0x00)||(sector[0x0F]!=0x01)||(sector[0x814]!=0x00)||(sector[0x815]!=0x00)||(sector[0x816]!=0x00)||(sector[0x817]!=0x00)||(sector[0x818]!=0x00)||(sector[0x819]!=0x00)||(sector[0x81A]!=0x00)||(sector[0x81B]!=0x00)) {
      canbetype1 = 0;
    }
  }
  if((sector[0x0]!=sector[0x4])||(sector[0x1]!=sector[0x5])||(sector[0x2]!=sector[0x6])||(sector[0x3]!=sector[0x7])) {
    canbetype2 = 0;
    canbetype3 = 0;
    if(!canbetype1) return 0;
  }
  myedc = ecm_edc_computeblock(0, sector, 0x808);
  if(canbetype2) if((sector[0x808]!=((myedc>>0)&0xFF))||(sector[0x809]!=((myedc>>8)&0xFF))||(sector[0x80A]!=((myedc>>16)&0xFF))||(sector[0x80B]!=((myedc>>24)&0xFF))) {
    canbetype2 = 0;
  }
  myedc = ecm_edc_computeblock(myedc, sector + 0x808, 8);
  if(canbetype1) if((sector[0x810]!=((myedc>>0)&0xFF))||(sector[0x811]!=((myedc>>8)&0xFF))||(sector[0x812]!=((myedc>>16)&0xFF))||(sector[0x813]!=((myedc>>24)&0xFF))) {
    canbetype1 = 0;
  }
  myedc = ecm_edc_computeblock(myedc, sector + 0x810, 0x10C);
  if(canbetype3) if((sector[0x91C]!=((myedc>>0)&0xFF))||(sector[0x91D]!=((myedc>>8)&0xFF))||(sector[0x91E]!=((myedc>>16)&0xFF))||(sector[0x91F]!=((myedc>>24)&0xFF))) {
    canbetype3 = 0;
  }
  if(canbetype1) { if(!(ecm_ecc_generate(sector, 0, sector + 0x81C))) { canbetype1 = 0; } }
  if(canbetype2) { if(!(ecm_ecc_generate(sector - 0x10, 1, sector + 0x80C))) { canbetype2 = 0; } }
  if(canbetype1) return 1;
  if(canbetype2) return 2;
  if(canbetype3) return 3;
  return 0;
}

static void ecm_write_type_count(FILE *out, unsigned type, unsigned count) {
  count--;
  fputc(((count >= 32) << 7) | ((count & 31) << 2) | type, out);
  count >>= 5;
  while(count) {
    fputc(((count >= 128) << 7) | (count & 127), out);
    count >>= 7;
  }
}

static unsigned ecm_mycounter_analyze;
static unsigned ecm_mycounter_encode;
static unsigned ecm_mycounter_total;
static progress_callback ecm_progress_cb = NULL;

static void ecm_resetcounter(unsigned total, progress_callback callback) {
  ecm_mycounter_analyze = 0;
  ecm_mycounter_encode = 0;
  ecm_mycounter_total = total;
  ecm_progress_cb = callback;
}

static void ecm_setcounter_analyze(unsigned n) {
  if((n >> 20) != (ecm_mycounter_analyze >> 20)) {
    if (ecm_progress_cb) ecm_progress_cb(n, ecm_mycounter_total, 0);
  }
  ecm_mycounter_analyze = n;
}

static void ecm_setcounter_encode(unsigned n) {
  if((n >> 20) != (ecm_mycounter_encode >> 20)) {
    if (ecm_progress_cb) ecm_progress_cb(n, ecm_mycounter_total, 1);
  }
  ecm_mycounter_encode = n;
}

static unsigned ecm_in_flush(unsigned edc, unsigned type, unsigned count, FILE *infile, FILE *outfile) {
  unsigned char buf[2352];
  ecm_write_type_count(outfile, type, count);
  if(!type) {
    while(count) {
      unsigned b = count;
      if(b > 2352) b = 2352;
      fread(buf, 1, b, infile);
      edc = ecm_edc_computeblock(edc, buf, b);
      fwrite(buf, 1, b, outfile);
      count -= b;
      ecm_setcounter_encode(ftell(infile));
    }
    return edc;
  }
  while(count--) {
    switch(type) {
    case 1:
      fread(buf, 1, 2352, infile);
      edc = ecm_edc_computeblock(edc, buf, 2352);
      fwrite(buf + 0x00C, 1, 0x003, outfile);
      fwrite(buf + 0x010, 1, 0x800, outfile);
      ecm_setcounter_encode(ftell(infile));
      break;
    case 2:
      fread(buf, 1, 2336, infile);
      edc = ecm_edc_computeblock(edc, buf, 2336);
      fwrite(buf + 0x004, 1, 0x804, outfile);
      ecm_setcounter_encode(ftell(infile));
      break;
    case 3:
      fread(buf, 1, 2336, infile);
      edc = ecm_edc_computeblock(edc, buf, 2336);
      fwrite(buf + 0x004, 1, 0x918, outfile);
      ecm_setcounter_encode(ftell(infile));
      break;
    }
  }
  return edc;
}

static unsigned char ecm_inputqueue[1048576 + 4];

int ecm_ecmify(FILE *infile, FILE *outfile, progress_callback callback) {
  unsigned inedc = 0;
  int curtype = -1;
  int curtypecount = 0;
  int curtype_in_start = 0;
  int detecttype;
  int incheckpos = 0;
  int inbufferpos = 0;
  int intotallength;
  int inqueuestart = 0;
  int dataavail = 0;

  fseek(infile, 0, SEEK_END);
  intotallength = ftell(infile);
  ecm_resetcounter(intotallength, callback);

  fputc('E', outfile);
  fputc('C', outfile);
  fputc('M', outfile);
  fputc(0x00, outfile);

  for(;;) {
    if((dataavail < 2352) && (dataavail < (intotallength - inbufferpos))) {
      int willread = intotallength - inbufferpos;
      if(willread > ((sizeof(ecm_inputqueue) - 4) - dataavail)) willread = (sizeof(ecm_inputqueue) - 4) - dataavail;
      if(inqueuestart) {
        memmove(ecm_inputqueue + 4, ecm_inputqueue + 4 + inqueuestart, dataavail);
        inqueuestart = 0;
      }
      if(willread) {
        ecm_setcounter_analyze(inbufferpos);
        fseek(infile, inbufferpos, SEEK_SET);
        fread(ecm_inputqueue + 4 + dataavail, 1, willread, infile);
        inbufferpos += willread;
        dataavail += willread;
      }
    }
    if(dataavail <= 0) break;
    if(dataavail < 2336) {
      detecttype = 0;
    } else {
      detecttype = ecm_check_type(ecm_inputqueue + 4 + inqueuestart, dataavail >= 2352);
    }
    if(detecttype != curtype) {
      if(curtypecount) {
        fseek(infile, curtype_in_start, SEEK_SET);
        inedc = ecm_in_flush(inedc, curtype, curtypecount, infile, outfile);
      }
      curtype = detecttype;
      curtype_in_start = incheckpos;
      curtypecount = 1;
    } else {
      curtypecount++;
    }
    switch(curtype) {
    case 0: incheckpos += 1; inqueuestart += 1; dataavail -= 1; break;
    case 1: incheckpos += 2352; inqueuestart += 2352; dataavail -= 2352; break;
    case 2: incheckpos += 2336; inqueuestart += 2336; dataavail -= 2336; break;
    case 3: incheckpos += 2336; inqueuestart += 2336; dataavail -= 2336; break;
    }
  }
  if(curtypecount) {
    fseek(infile, curtype_in_start, SEEK_SET);
    inedc = ecm_in_flush(inedc, curtype, curtypecount, infile, outfile);
  }

  ecm_write_type_count(outfile, 0, 0);

  fputc((inedc >> 0) & 0xFF, outfile);
  fputc((inedc >> 8) & 0xFF, outfile);
  fputc((inedc >> 16) & 0xFF, outfile);
  fputc((inedc >> 24) & 0xFF, outfile);

  // Final progress update to show 100%
  if (ecm_progress_cb) {
      ecm_progress_cb(ecm_mycounter_total, ecm_mycounter_total, 0);
      ecm_progress_cb(ecm_mycounter_total, ecm_mycounter_total, 1);
  }

  return 0;
}