/*kate: syntax ANSI C89;*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <sys/mman.h>
#include <endian.h>
	
#include "encode.h"
#include "huff.h"

#define BUFF_LEN 1048576
  
struct WRITE_BUFF {
	byte 	*data;
	size_t 	index;
};

struct FILE_BUFF {
	byte 	*data;
	size_t	index;
	size_t 	len;
};


int letter_compare(const void* pa, const void* pb){
	struct Letterdata *a = (struct Letterdata*)pa;
	struct Letterdata *b = (struct Letterdata*)pb;
	if( a->freq == 0 )
		return 1;
	if( b->freq == 0 )
		return -1;
	return a->freq - b->freq;
}

/* pushes letters with a value to the top */
/* BUG:  doesnt work sometimes... */
/* I dont even know if the bug is still here */
static 
uint32_t push_letters(struct Letterdata LETTERS[CHAR_MAX])
{
	uint32_t j = 0;
	
	qsort(LETTERS, CHAR_MAX, sizeof(struct Letterdata), letter_compare);
	
	while( j < CHAR_MAX && LETTERS[j].freq != 0)
		j++;
	
	return j;
}

static
uint32_t count_letters(struct FILE_BUFF fin, struct Letterdata LETTERS[CHAR_MAX])
{
	byte ch;
	int i;
	size_t len = fin.len;
	
	memset(LETTERS, 0, CHAR_MAX * sizeof(struct Letterdata));
	
	for( i = 0 ; i < CHAR_MAX ; i++ ){
		LETTERS[i].ch = i;
	}
	ch = EOF;
	
	while( len-- ){
		ch = fin.data[ fin.index++ ];
		LETTERS[ch].freq += 1;
	}
	LETTERS[ch].freq = 1;/*adds eof*/
	
	return push_letters(LETTERS);
}

static
void sort_tree(struct Node *nodes[CHAR_MAX], struct Letterdata LETTERS[CHAR_MAX], int32_t count)
{
	int32_t i, j, a, b;
	struct Node 		*tmp_node;
	struct Letterdata 	tmp_letter;
	
	for( i = 0; i < count; i++ )
	{
		a = LETTERS[i].freq;
		if( a == -1 ){
			/*1 is added so that nodes dont form a giant string of nodes*/
			a = nodes[i]->sum + 1;
		}
		
		for( j = i + 1; j < count; j++ )
		{
			b = LETTERS[j].freq;
			if( b == -1 ){
				b = nodes[j]->sum + 1;
			}

			if( a < b ){
				tmp_letter = LETTERS[j];
				LETTERS[j] = LETTERS[i];
				LETTERS[i] = tmp_letter;
				
				tmp_node = nodes[i];
				nodes[i] = nodes[j];
				nodes[j] = tmp_node;
			}
		}
	}
}

static
struct Node *create_tree(int32_t letter_count, struct Letterdata LETTERS[CHAR_MAX])
{
	struct Node *nodes[CHAR_MAX];
	struct Node *temp = NULL;
	int32_t i, j;
	
	letter_count -= 1;
	
	sort_tree(nodes, LETTERS, letter_count);
	for( i = letter_count; i > 0; i-- )
	{
		j = i - 1;
		
		temp = malloc(sizeof(struct Node));
		if( temp == NULL ){
			return NULL;
		}
		
		if( LETTERS[i].freq == -1 )
		{
			temp->sum += nodes[i]->sum;
			temp->next[0] = nodes[i];
		}
		else{
			temp->sum += LETTERS[i].freq;
			temp->ch[0] = LETTERS[i].ch;
			temp->next[0] = NULL;
		}

		if( LETTERS[j].freq == -1 )
		{
			temp->sum += nodes[j]->sum;
			temp->next[1] = nodes[j];
		}
		else{
			temp->sum += LETTERS[j].freq;
			temp->ch[1] = LETTERS[j].ch;
			temp->next[1] = NULL;
		}
		nodes[j] = temp;
		LETTERS[i].freq = -1;
		LETTERS[j].freq = -1;

		sort_tree(nodes, LETTERS, i);
	}
	
	return nodes[0];
}

/*Simple function to check if the byte should be written to the file*/
__attribute__((always_inline)) static __inline__ 
void check_write(FILE *fout, byte *c, byte *cindex, struct WRITE_BUFF *Buff)
{
	if( *cindex >= 8 ){
		Buff->data[Buff->index] = *c;
		
		Buff->index += 1;
		if( Buff->index == BUFF_LEN ){
			fwrite(Buff->data, sizeof(byte), BUFF_LEN, fout);
			Buff->index = 0;
		}
		
		*cindex = 0;
		*c = 0;
	}
}


/*Function for creating the codes used to write */
/*NOTE:
	bitwise with 8-bit and 16-bit variables happens like this:

	16 bit  | 0xff00
			| OR
	8  bit  | 0x00
			| = 0x00

	16 bit  | 0x00ff
			| OR
	8  bit  | 0x00
			| = 0xff

	0x00  ff		- 16-bit number
	0x    00		- 8 bit number
	
	The bitwise between 8 bit and 16 bit happen on the last bits of the 16 bit number
	(Makes sense when you think of how the registers are positioned).
*/
static
void init_codes(const struct Node *head, uint32_t codes[CHAR_MAX][2], uint32_t temp_codes, uint32_t depth)
{
	uint32_t p;
	
	for( p = 0; p < 2; p += 1 )
	{
		temp_codes |= ((p<<31)>>depth);
		
		if( head->next[p] == NULL ){
			codes[ head->ch[p] ][0] = temp_codes;
			codes[ head->ch[p] ][1] = depth+1;
		}
		else{
			init_codes(head->next[p], codes, temp_codes, depth+1);
		}
	}
}


/*
 Function that writes the huffman tree to the output file.
 */
static 
void write_tree(FILE *fout, byte *c, byte *cindex, const struct Node *head, struct WRITE_BUFF *Buff)
{
	byte p;
	
	for( p = 0; p < 2; p += 1 )
	{
		check_write(fout, c, cindex, Buff);
		
		if( head->next[p] == NULL )
		{
			*c |= WRITE_BIT(1, *cindex);
			*cindex += 1;
			
			*c |= (head->ch[p]>>*cindex);
			
			Buff->data[ Buff->index++ ] = *c;
			*c = (head->ch[p]<<(__CHAR_BIT__-*cindex));
		}
		else{
			*cindex += 1; /*leaves the bit we were at at 0*/
			write_tree(fout, c, cindex, head->next[p], Buff);
		}
	}
}

/*
 Main encoding function. Includes logic for writing the huffman codes to the file.
 */
int encode(FILE *fin, FILE *fout)
{
	struct Letterdata LETTERS[CHAR_MAX];
	struct WRITE_BUFF out_buff;
	struct FILE_BUFF in_buff;
	byte c = 0, cindex = 0;

	uint32_t codes[CHAR_MAX][2] = {0}; /*List of huffman codes [0] == code [1] == code length*/
	uint32_t temp_codes = 0;
	
	register byte 	  cread;		/*Byte thats being read*/
	register uint32_t code_index;	/*How many bits we are into a code*/
	uint64_t 	len;			/*File length*/
	
	struct Node *head = NULL;		/*Huffman tree for construction*/
	
	if( fin == NULL || fout == NULL ){
		errno = EBADF;
		return -1;
	}
	
	out_buff.index = 0;
	out_buff.data = malloc(sizeof(byte) * BUFF_LEN);
	
	fseek(fin, 0l, SEEK_END);
	in_buff.len = len = ftell(fin);
	rewind(fin);
	
	in_buff.data = mmap(NULL, in_buff.len, PROT_READ, MAP_SHARED, fileno(fin), 0);
	in_buff.index = 0;
	
	head = create_tree( count_letters(in_buff, LETTERS), LETTERS );
	in_buff.index = 0;

	{
		uint64_t write_len = htobe64(len);
		fwrite(&write_len, sizeof(uint64_t), 1, fout);
	}

	init_codes(head, codes, temp_codes, 0);
	write_tree(fout, &c, &cindex, head, &out_buff);
	
	if( do_print_tree ){ print_tree(head, 1, 0); }
	free_tree(head);
	
	while( len-- > 0 )
	{
		cread = in_buff.data[ in_buff.index++ ];
		
		/*Moves bits into remaining space*/
		c |= (codes[cread][0]>>24)>>cindex;
		code_index = __CHAR_BIT__ - cindex;
		
		/*Write all bits into the buffer.
		 This shit is magic for me now*/
		if( codes[cread][1] > code_index ){
			do{
				out_buff.data[out_buff.index] = c;
				out_buff.index += 1;
				if( out_buff.index == BUFF_LEN ){
					fwrite(out_buff.data, sizeof(byte), BUFF_LEN, fout);
					out_buff.index = 0;
				}
				
				c = (codes[cread][0]<<code_index)>>24;
				
				cindex = codes[cread][1] - code_index;
				code_index += 8;
			}
			while(codes[cread][1] > code_index);
		}
		else{
			cindex += codes[cread][1];
			check_write(fout, &c, &cindex, &out_buff);
		}
	}
	
	if( cindex > 0 ){
		out_buff.data[out_buff.index] = c;
		out_buff.index += 1;
		if( out_buff.index == BUFF_LEN ){
			fwrite(out_buff.data, sizeof(byte), BUFF_LEN, fout);
			out_buff.index = 0;
		}
	}
	fwrite(out_buff.data, sizeof(byte), out_buff.index+1, fout);
	
	free(out_buff.data);
	munmap(in_buff.data, in_buff.len);
	return 0;
}

/*
|1|1|1|1| | | | |
		|0|0|0|	

|_______| (cindex)
		|_______| (8 - cindex)
		|_____| (codes[cread][1])
            
            
|1|1|1|1|1|1| | |
            |0|0|0|0|0|0|0|	
            
			|___| (8 - cindex)

|___________| (cindex)
|_______________| 8
|_| 		|_____________| codes[cread][1]
                |_________| codes[cread][1] - (8 - cindex )
				
*/

