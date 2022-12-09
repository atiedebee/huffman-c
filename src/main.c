/*kate: syntax ANSI C89;*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "huff.h"
#include "encode.h"
#include "decode.h"

const char endoffile = EOF;
char do_print_tree = 0;

void free_tree(struct Node *head)
{
	if( head->next[0] != NULL ){
		free_tree(head->next[0]);
	}
	if( head->next[1] != NULL ){
		free_tree(head->next[1]);
	}
	free(head);
	head = NULL;
}


void padd(int32_t depth)
{
	const char *padding = "    ";
	int32_t i;
	for( i = 0; i < depth; i++ ){
		printf("%s", padding);
	}
}

void print_tree(struct Node *head, int32_t depth, int32_t isabove)
{
	if( head->next[0] != NULL ){
		print_tree(head->next[0], depth+1, 1);
	}
	else{
		padd(depth);
		printf("/--");
		PRINT_CHAR(head->ch[0]);
		putchar('\n');
	}

	padd(depth - 1);
	if( isabove == 1 ){
		printf("/--<\n");
	} else if( isabove == 0 ){
		printf("---<\n");
	} else {
		printf("\\--<\n");
	}

	if( head->next[1] != NULL ){
		print_tree(head->next[1], depth+1, -1);
	} else {
		padd(depth);
		
		printf("\\--");
		PRINT_CHAR(head->ch[1]);
		putchar('\n');
	}
}

enum Mode parse_args(int argc, char **argv, FILE **fin, FILE **fout)
{
	const char *optstring = "c:d:o:";
 	char *fin_name = NULL, *fout_name = NULL;
	char opt;
	enum Mode mode = NO_MODE;
	while( (opt = getopt(argc, argv, optstring)) != -1 ){
		switch(opt)
		{
			case 'c':
				if( mode != NO_MODE )
					return NO_MODE;
				
				mode = COMPRESS_MODE;
				fin_name = optarg;
				break;
			
			case 'd':
				if( mode != NO_MODE )
					return NO_MODE;
				
				mode = DECOMPRESS_MODE;
				fin_name = optarg;
				break;
			
			case 'o':
				if( fout_name != NULL ){
					fprintf(stderr, "Improper usage, only one output file can be selected at a time\n");
					return NO_MODE;
				}
				fout_name = optarg;
				break;
		}
	}
	
	
	if(fin_name != NULL)
	{
		*fin = fopen(fin_name, "rb");
		if( *fin == NULL){
			fprintf(stderr, "\nFailed opening %s\n", fin_name);
			return NO_MODE;
		}
	}else{
		fprintf(stderr, "\nNo input file specified.\n");
		return NO_MODE;
	}
	*fout = stdout;
	
	if(fout_name != NULL)
	{
		*fout = fopen(fout_name, "wb");
		if( *fout == NULL){
			fprintf(stderr, "\nFailed opening '%s'.\n", fout_name);
			return NO_MODE;
		}
	}
	return mode;
}


int main(int argc, char **argv)
{
	FILE *fin = NULL, *fout = NULL;
	
	switch( parse_args(argc, argv, &fin, &fout)){
		case DECOMPRESS_MODE:
			decode(fin, fout);
			break;
		case COMPRESS_MODE:
			encode(fin, fout);
			break;
		case NO_MODE:
			puts("\nUsage:\n"
			"[-c] encode a file\n"
			"[-d] deencode a file\n"
			"[-o] select an output file\n");
			break;
	}
	
	if( fin != NULL  ) fclose(fin);
	if( fout != NULL ) fclose(fout);
	return 0;
}
