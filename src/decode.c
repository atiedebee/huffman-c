/*kate: syntax ANSI C89;*/
#include "decode.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "huff.h"

__attribute__((always_inline)) static __inline__ 
void check_read(FILE *fin, byte *c, byte *cindex)
{
	if( *cindex >= 8 ){
		*c = fgetc(fin);
		*cindex = 0;
	}
}

static 
struct Node *read_tree(FILE *fin, byte *c, byte *cindex)
{
	struct Node *node = malloc(sizeof(struct Node));
	byte p, i;
	
	for( p = 0; p < 2; p++ )
	{
		check_read(fin, c, cindex);
		
		if( READ_BIT(*c, *cindex) == 0 ){
			*cindex += 1;
			node->next[p] = read_tree(fin, c, cindex);
			continue;
		}

		node->next[p] = NULL;
		*cindex += 1;
		for( i = 0; i < 8; i++ ){
			check_read(fin, c, cindex);
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
	byte c = 0, cindex = 8;
	byte p;
	uint64_t end;
	
	fread(&end, sizeof(uint64_t), 1, fin);
	
	head = read_tree(fin, &c, &cindex);
	parse_node = head;
	
	if( do_print_tree ){ print_tree(head, 1, 0); }

/* 
 TODO
 create lookup table for codes
 */
	while( end > 0 )
	{
		check_read(fin, &c, &cindex);
		p = READ_BIT(c, cindex);
		cindex += 1;
		
		if( parse_node->next[p] == NULL ){
			
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
	return 0;
}
