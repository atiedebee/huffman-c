/*kate: syntax ANSI C89;*/
#include "decode.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/mman.h>

#include "huff.h"


struct FILE_BUFF {
	byte 	*data;
	size_t 	index;
	size_t 	len;
};

__attribute__((always_inline)) static __inline__ 
void check_read(struct FILE_BUFF *fbuff, byte *c, byte *cindex)
{
	if( *cindex >= 8 ){
		*c = fbuff->data[ fbuff->index++ ];
		*cindex = 0;
	}
}

static 
struct Node *read_tree(struct FILE_BUFF *fbuff, byte *c, byte *cindex)
{
	struct Node *node = malloc(sizeof(struct Node));
	byte p, i;
	
	for( p = 0; p < 2; p++ )
	{
		check_read(fbuff, c, cindex);
		
		if( READ_BIT(*c, *cindex) == 0 ){
			*cindex += 1;
			node->next[p] = read_tree(fbuff, c, cindex);
			continue;
		}

		node->next[p] = NULL;
		*cindex += 1;
		for( i = 0; i < 8; i++ ){
			check_read(fbuff, c, cindex);
			node->ch[p] |= WRITE_BIT(READ_BIT(*c, *cindex), i); /*copy bits over*/
			*cindex += 1;
		}
	}

	return node;
}

int32_t decode(FILE *fin, FILE *fout)
{
	struct Node *head = NULL;
	struct Node *parse_node = NULL;
	struct FILE_BUFF fbuff;
	byte c = 0, cindex = 8;
	byte p;
	uint64_t end;
	
	fread(&end, sizeof(uint64_t), 1, fin);
	
	fseek(fin, 0l, SEEK_END);
	fbuff.len = ftell(fin);
	fbuff.data = mmap(NULL, fbuff.len, PROT_READ, MAP_SHARED, fileno(fin), 0);
	fbuff.index = 0;
	
	head = read_tree(&fbuff, &c, &cindex);
	parse_node = head;
	
	if( do_print_tree ){ print_tree(head, 1, 0); }

	while( end > 0 )
	{
		check_read(&fbuff, &c, &cindex);
		p = READ_BIT(c, cindex);
		cindex += 1;
		
		if( parse_node->next[p] == NULL )
		{
			if( parse_node->ch[p] == *(byte*)&endoffile )
				break;
			
			fputc(parse_node->ch[p], fout);
			
			parse_node = head;
			end--;
		} 
		else{
			parse_node = parse_node->next[p];
		}
	}
	
	free_tree(head);
	munmap(fbuff.data, fbuff.len);
	return 0;
}
