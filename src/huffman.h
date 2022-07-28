#ifndef __HUFMAN_H__
#define __HUFMAN_H__

#include <stdio.h>
#include <stdint.h>

int32_t encode(FILE *fin, FILE *fout);
int32_t decode(FILE *fin, FILE *fout);

#endif
