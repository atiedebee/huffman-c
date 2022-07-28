#ifndef ENCODE_H
#define ENCODE_H

#include "huff.h"

struct Letterdata {
	byte 	ch;
	int64_t	freq;
};

extern struct Letterdata g_LETTERS[CHAR_MAX];

int32_t encode(FILE *fin, FILE *fout);


#endif
