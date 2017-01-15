// main.cpp
/*
	Copyright (c) 2006, Toby Oxborrow <www.oxborrow.net>
	All rights reserved. See LICENCE for details.
	--
	Toby's Huffman Compression - Main
	A wrapper for the THuffman class so hu-mans can use it.
*/
#include "huffman.h"


void Usage(char *argv[])
{
	printf("Usage: %s -e|d [input file] [output file]\n", argv[0]);
}

void HandleErr(unsigned int err)
{
	switch(err) {
		case 1:
			puts("Error opening input file.");
			break;
		case 2:
			puts("Error opening output file.");
			break;
		case 3:
			puts("Empty input file");
			break;
		case 4:
			puts("Read error.");
			break;
		case 5:
			puts("Write error.");
			break;
		default:
			puts("Unknown error.");
	}
}


int main(int argc, char *argv[])
{
	if(argc != 4) {
		Usage(argv);
	} else {
		unsigned int err;
		THuffman huff;
		if(strcmp(argv[1], "-e") == 0) {
			err = huff.Encode(argv[2], argv[3]);
		} else if (strcmp(argv[1], "-d") == 0) {
			err = huff.Decode(argv[2], argv[3]);
		} else {
			Usage(argv);
		}
		if(err)
			HandleErr(err);
	}
	return 0;
}
