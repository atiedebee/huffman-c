#ifndef __HUFF_H__
#define __HUFF_H__

#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

#define READ_BIT(b, i) ( (b&((1<<7)>>i)) > 0)
#define WRITE_BIT(b, i) ((b<<7)>>i)
#define SWAP(x, y) ((x)^=(y), (y)^=(x), (x)^=(y))

#define CHAR_MAX (UINT8_MAX)

#define PRINT_CHAR(x) \
do{								\
if( isprint(x) )		\
	printf("%c", x);			\
else							\
	printf("%#x", x);			\
}while(0);

typedef uint8_t byte;

enum Mode {COMPRESS_MODE, DECOMPRESS_MODE, NO_MODE};

struct Node {
	int64_t 	sum;
	struct Node *next[2];
	byte 		ch[2];
};

extern const char endoffile;
extern char do_print_tree;

void padd(int32_t depth);
void print_tree(struct Node *head, int32_t depth, int32_t isabove);
void free_tree(struct Node *head);

#endif
