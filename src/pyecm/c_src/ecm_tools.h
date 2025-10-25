#ifndef ECM_TOOLS_H
#define ECM_TOOLS_H

#include <stdio.h>

typedef void (*progress_callback)(unsigned int current, unsigned int total, int type);

void ecm_eccedc_init(void);
int ecm_ecmify(FILE *infile, FILE *outfile, progress_callback callback);

void unecm_eccedc_init(void);
int unecm_unecmify(FILE *infile, FILE *outfile, progress_callback callback);

#endif