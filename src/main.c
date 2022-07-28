/*kate: syntax ANSI C89;*/
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

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
 	char fin_name[256] = {0}, fout_name[256] = {0};
	int32_t i, mode_changes = 0;
	enum Mode mode = NO_MODE;
	
	for( i = 1; i < argc; i++ )
	{
		if( strncmp(argv[i], "-c", 2) == 0)
		{
			mode = COMPRESS_MODE;
			mode_changes += 1;
		}
		else if( strncmp(argv[i], "-d", 2) == 0)
		{
			mode = DECOMPRESS_MODE;
			mode_changes += 1;
		}else if( strncmp(argv[i], "-o", 2) == 0)
		{
			if( i+1 >= argc ){
				fprintf(stderr, "\nYou need to provide an output file\n");
				return NO_MODE;
			}
			strncpy(fout_name, argv[i+1], 255);
			i += 1;
		}else if( strncmp(argv[i], "-p", 2) == 0)
		{
			do_print_tree = 1;
		}else{
			strncpy(fin_name, argv[i], 255);
		}
	}
	
	
	if(fin_name[0] != 0)
	{
		if( mode == COMPRESS_MODE ){
			*fin = fopen(fin_name, "r");
		}else{
			*fin = fopen(fin_name, "rb");
		}
		
		if( *fin == NULL){
			fprintf(stderr, "\nFailed opening %s\n", fin_name);
			return NO_MODE;
		}
	}else{
		fprintf(stderr, "\nNo input file specified.\n");
		return NO_MODE;
	}
	
	*fout = stdout;
	if(fout_name[0] != 0)
	{
		if( mode == COMPRESS_MODE ){
			*fout = fopen(fout_name, "wb");
		}
		else{
			*fout = fopen(fout_name, "w");
		}
		
		if( *fout == NULL){
			fprintf(stderr, "\nFailed opening '%s'.\n", fout_name);
			return NO_MODE;
		}
	}
	
	if( mode_changes > 1 ){
		fprintf(stderr, "\nYou can either encode OR deencode one file, not both!\n");
		return NO_MODE;
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
