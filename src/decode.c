/*kate: syntax ANSI C89;*/
#include "decode.h"

#include <bits/types/struct_timeval.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/mman.h>
#include <endian.h>
#include <sys/time.h>

#include "huff.h"

#define PRINTF_BYTE_FORMAT "%d%d%d%d%d%d%d%d"
#define PRINTF_BYTE_FORMAT_FUNC(b) \
(b & (1<<7)) != 0,\
(b & (1<<6)) != 0,\
(b & (1<<5)) != 0,\
(b & (1<<4)) != 0,\
(b & (1<<3)) != 0,\
(b & (1<<2)) != 0,\
(b & (1<<1)) != 0,\
(b & (1)) != 0


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
struct Node *read_tree(struct FILE_BUFF *fbuff, byte *c, byte *cindex, uint64_t *min_depth, uint64_t depth)
{
	struct Node *node = malloc(sizeof(struct Node));
	byte p, i;
	
	for( p = 0; p < 2; p++ )
	{
		check_read(fbuff, c, cindex);
		
		if( READ_BIT(*c, *cindex) == 0 ){
			*cindex += 1;
			node->next[p] = read_tree(fbuff, c, cindex, min_depth, depth + 1);
			continue;
		}

		if( depth < *min_depth ){
			*min_depth = depth;
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

void init_min_depth_nodes(struct Node* node, struct Node **min_depth_nodes, uint64_t path, uint64_t depth, const uint64_t min_depth){
	if( depth == min_depth ){
		min_depth_nodes[path] = node;
	}else{
		init_min_depth_nodes(node->next[0], min_depth_nodes, path, depth+1, min_depth);
		init_min_depth_nodes(node->next[1], min_depth_nodes, path | (1 << ( min_depth - depth - 1)), depth+1, min_depth);
	}
}


int32_t decode(FILE *fin, FILE *fout)
{
	struct Node *head = NULL;
	struct Node *parse_node = NULL;
	struct Node **min_depth_nodes;
	struct FILE_BUFF fbuff;
	uint64_t min_depth = UINT64_MAX;
	byte c = 0, cindex = 8;
	byte p;
	uint64_t end;

	fread(&end, sizeof(uint64_t), 1, fin);
	end = be64toh(end);

	fbuff.len = end + sizeof(uint64_t);
	fbuff.data = mmap(NULL, fbuff.len, PROT_READ, MAP_SHARED, fileno(fin), 0);
	fbuff.index = sizeof(uint64_t);
	
	head = read_tree(&fbuff, &c, &cindex, &min_depth, 0);
	parse_node = head;

	min_depth_nodes = malloc(sizeof(void*) * (1<<min_depth));
	init_min_depth_nodes(head, min_depth_nodes, 0, 0, min_depth);


	if( do_print_tree ){ print_tree(head, 1, 0); }

	while( end > 0 )
	{
		uint64_t min_depth_node_index = (c & (0xFF>>cindex));
		uint64_t temp_cindex = cindex + min_depth;

		if(temp_cindex >= 8){
			c = fbuff.data[fbuff.index++];
			temp_cindex -= 8;

			min_depth_node_index <<= temp_cindex;
			min_depth_node_index |= c >> (8 - temp_cindex);
		}else{
			min_depth_node_index >>= (8 - cindex - min_depth);
		}


		cindex = temp_cindex;
		parse_node = min_depth_nodes[min_depth_node_index];


		check_read(&fbuff, &c, &cindex);
		p = READ_BIT(c, cindex);
		cindex += 1;

		while(parse_node->next[p] != NULL){
			parse_node = parse_node->next[p];
			check_read(&fbuff, &c, &cindex);
			p = READ_BIT(c, cindex);
			cindex += 1;
		}
		if( parse_node->ch[p] == *(byte*)&endoffile )
			break;

		fputc(parse_node->ch[p], fout);
		parse_node = head;
		end--;
	}

	free(min_depth_nodes);

	free_tree(head);
	munmap(fbuff.data, fbuff.len);
	return 0;
}

/*
NOTE: decode

path can be read at once:

c:	|1|0|0|0|1|0|1|1|
	|_______|			cindex (4)
	|_____|				min_depth (3)
c:	|1|0|0|0|1|0|1|1|
		    |1|1|1|1|1|1|1|1|
---------------------------- & 0xFF >> cindex
c:	|0|0|0|0|1|0|1|1|
---------------------------- >> (8 - cindex - min_depth)
c:  |0|0|0|0|0|1|0|1|		(path)

path = (c & (0xFF >> cindex)) >> (8 - cindex - min_depth)



path cant be read at once:

	|_______________| |_______________|
c:	|1|0|0|0|1|0|1|1| |1|0|0|0|1|0|1|1|
	|_____________|						cindex (6)
	|_____|								min_depth (3)

c:	|1|0|0|0|1|0|1|1| |1|0|0|0|1|0|1|1|
				  |1|1|1|1|1|1|1|1|
---------------------------------------- & 0xFF >> cindex
c:	|0|0|0|0|0|0|0|1| |1|0|0|0|1|0|1|1|
	|0|0|0|0|0|0|0|1|						path
	|_______________| |___|					cindex + min_depth
	|___|									cindex + min_depth - 8

	path << (cindex + min_depth - 8)
	|0|0|0|0|0|1|0|0|						path

c:	|1|0|0|0|1|0|1|1|
----------------------------------------- >> 8 - cindex - min_depth + 8
c:				|1|0|
----------------------------------------- | path
	|0|0|0|0|0|1|1|0|						path





		*/
