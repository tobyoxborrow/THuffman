// huffman.h
/*
	Copyright (c) 2006, Toby Oxborrow
	All rights reserved.
	--
	Toby's Huffman Compression - THuffman
	The main class. Converts plain text to huffman encoded strings and
	visa-versa.

	Limitations: The header produced is quite large and so currently not
	suitable on very short strings with low repetition.

	Usage Example:

	THuffman huff;
	string encoded = huff.Encode("peter piper picked a peck of picked peppers");
	// probably best not to put binary data to a console, oh well...
	puts(encoded.c_str());
	// this won't make your console cry though...
	puts(huff.Decode(encoded).c_str());

	huff.Encode("c:\some\plain.file", "c:\some\encoded.file");
	huff.Decode("c:\some\encoded.file", "c:\some\decoded.file");
*/
#pragma once
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include "bit_buffer.h"
#include "huffman_btree.h"


class THuffman {

	private:

		// -- for encoding only:
		std::vector<THuffmanBTree*> forest;
		std::map<const char, TBitBuffer*> bitTable;
		// freqTable is a convenience for constructing the header
		std::map<const char, unsigned long> freqTable;
		// -- for decoding only:
		std::map<const std::string, const char> codeTable;	// reverse of bitTable
		std::map<const std::string, const char> codeTableCache;
		// -- both modes:
		std::string plainText;
		TBitBuffer encodedText;


		// high-level private functions
		void					_PopulateForest();
		void					_ReadHeader();
		void					_BuildBitTree();
		void					_BuildBitTable();
		void					_WriteHeader(const unsigned long);
		std::string				_EncodeText();
		void					_DecodeText();


		// utility functions
		void					__CleanUp();
		void					__DebugForest();


		// a compare function to use with sort()
		static bool		__TreeFreqCmp(THuffmanBTree *a, THuffmanBTree *b)
			{ return a->GetRootFreq() < b->GetRootFreq(); }

	public:

		//THuffman() {}
		//~THuffman() {}

		// pass string, returns encoded/decoded result
		std::string		Encode(const std::string &);
		std::string		Decode(const std::string &);
		// filenames, output file will be encoded/decoded from input file
		int				Encode(const std::string &, const std::string &);
		int				Decode(const std::string &, const std::string &);
		// file handles, works as previous
		int				Encode(std::ifstream &, std::ofstream &);
		int				Decode(std::ifstream &, std::ofstream &);

};


inline std::string THuffman::Encode(const std::string & input)
{
	puts("Encode");
	plainText.assign(input);
	encodedText.Clear();
	printf("Plain Size:    %u bytes\n", plainText.size());

	_PopulateForest();			// modifies: forest, freqTable, bitTable
	//__DebugForest();
	_BuildBitTree();			// modifies: forest
	//__DebugForest();
	_BuildBitTable();			// modifies: bitTable
	printf("Characters:    %u\n", bitTable.size());

	// build the body first, so we can get the size and pass it to _GenerateHeader()
	std::string tmp = _EncodeText();
	_WriteHeader(tmp.size());
	printf("Header Size:   %u bits\n", encodedText.Size());
	printf("Body Size:     %u bits\n", tmp.size());
	encodedText.AppendBits(tmp);

	__CleanUp();

	printf("Total Size:    %u bytes\n", (encodedText.Size() / 8));
	return encodedText.ReadAllBytes();
}
inline std::string THuffman::Decode(const std::string & input)
{
	puts("Decode");
	plainText.clear();
	encodedText.AssignBytes(input);
	printf("Encoded Size:  %u bytes\n", (encodedText.Size() / 8));

	_ReadHeader();			// modifies: codeTable
	printf("Characters:    %u\n", codeTable.size());
	//__DebugForest();
	_DecodeText();

	__CleanUp();

	printf("Decoded Size:  %u bytes\n", plainText.size());
	return plainText;
}


inline std::string THuffman::_EncodeText()
{
	//puts("_EncodeText()");
	std::string r = "";
	unsigned int len = plainText.size();
	// walk through the input string, looking up each character as we go
	for(unsigned int i=0;i<len;i++)
		r.append(bitTable[plainText.at(i)]->ReadAllBits());
	return r;
}
// read the text one bit at a time. everytime we add one bit and look it up.
// if we can't find it, read another...
// this works because the bit codes are unique
// the codeTable.find(buf) is the slowest part of the whole process (encoding or decoding)
inline void THuffman::_DecodeText()
{
	//puts("_DecodeText()");
	unsigned long len = encodedText.Size();
	std::string buf = "";
	for(unsigned long i=0;i<len;i++) {
		buf.append(1, encodedText.ReadBit());
		// FIXME: slowest line in whole code:
		if(codeTable.find(buf) != codeTable.end()) {
			plainText.append(1, codeTable[buf]);
			buf.clear();
		}
	}
}


// using the given string, enter root nodes into the forest for each unique character
// where duplicates exist, increment totalFrequency
inline void THuffman::_PopulateForest()
{
	//puts("_PopulateForest()");

	/*
	-- implimentation note --
	for speed we populate 'freqTable' first as the map is small and uses
	simple data types, so it will be faster to look up existing
	characters compared to filling 'forest' and doing lookups on that
	which would require searching all the btrees (*very* costly)
	*/
	unsigned long len = plainText.size();
	for(unsigned long i=0;i<len;i++) {
		char character = plainText.at(i);
		if(freqTable.find(character) == freqTable.end()) {
			freqTable.insert(std::make_pair(character, 1));
		} else {
			freqTable[character] += 1;
		}
	}

	/*
	-- stupidity note --
	pointless as it is, a single character *could* be passed to us
	it's stupid because our header alone is bigger than that.
	but as i am a nice guy, we shall handle it none the less.
	here we add a second character so the bitTable builds correctly
	since the second char does not exist, it won't affect decoding
	but it will make our header a bit bigger (i'm nice but spiteful) :)
	it is unimportant what character we add,
	as long as its not the one that is actually there
	*/
	if(freqTable.size() == 1) {
		if(freqTable.begin()->first != 'a')
			freqTable['a'] = 1;
		else
			freqTable['b'] = 1;
	}

	// using 'freqTable' we can now quickly populate the forest
	std::map<const char, unsigned long>::iterator itr;
	for(itr = freqTable.begin(); itr != freqTable.end(); itr++) {
		THuffmanBTree *ht = new THuffmanBTree;		// new root node
		ht->Insert(itr->first, itr->second);
		forest.push_back(ht);
	}

	// sort the forest by totalFrequency, highest value at the end
	sort(forest.begin(), forest.end(), __TreeFreqCmp);
}


inline void THuffman::_WriteHeader(unsigned long bodySize)
{
	//puts("_WriteHeader()");

	/*
	-- header format --
	[total_letters]<letters>[byte_padding]
	<letters>:
		[letter][code_len][code]
	*/

	// we'll write directly to encodedText
	encodedText.Clear();
	encodedText.AppendByte(bitTable.size());		// how many letters

	// add each letter
	std::map<const char, TBitBuffer*>::iterator itr;
	for(itr = bitTable.begin(); itr != bitTable.end(); itr++) {
		encodedText.AppendByte(itr->first);
		encodedText.AppendNumber(itr->second->Size());
		encodedText.AppendBits(itr->second->ReadAllBits());
	}

	/*
	-- implimentation note --
	byte padding goes here, between header and body
	i think this is better than padding the end because to pad the end
	we have to know how much space is taken up by the body to store
	the length of padding at the end correctly.
	that value would likely require a byte to store (or more).
	this way we use at most a byte and the the added bonus that the
	decode() function can loop until it runs out of characters.
	the padding consists of 0's terminated by 1, which marks the start
	of the body
	*/
	encodedText.AppendPadding(encodedText.Size() + bodySize);
}
// nothing special to say here. does the opposite of _WriteHeader()
inline void THuffman::_ReadHeader()
{
	//puts("_ReadHeader()");
	unsigned char characters = encodedText.ReadByte();
	//printf("Header says there are %u characters:\n", characters);
	char character;
	unsigned long len;
	std::string charCode;
	for(unsigned char i=0;i<characters;i++) {
		character = encodedText.ReadByte();
		len = encodedText.ReadNumber();
		charCode = encodedText.ReadBits(len);
		codeTable.insert(std::make_pair(charCode, character));
		// this output is the same as encoding's _BuildBitTable()
		//printf("%c = %s\n", character, charCode.c_str());
	}
	encodedText.ReadPadding();
}


// using our 'forest', build a greedy tree of the character's frequency
// although this function has both erase() and sort(), under testing it isn't all that slow
inline void THuffman::_BuildBitTree()
{
	//puts("_BuildBitTree()");
	while(forest.size() > 1) {
		// take the 2 lowest frequency items and combine into a new forest node
		THuffmanBTree * lowestFreqNode1 = forest.at(0);
		THuffmanBTree * lowestFreqNode2 = forest.at(1);
		// we can remove them from the forest now
		forest.erase(forest.begin());
		forest.erase(forest.begin());
		//printf("1) %c : %u\n", lowestFreqNode1->GetRootLetter(), lowestFreqNode1->GetRootFreq());
		//printf("2) %c : %u\n", lowestFreqNode2->GetRootLetter(), lowestFreqNode2->GetRootFreq());
		// create a new place in the forest to keep them
		THuffmanBTree *foo = new THuffmanBTree;
		foo->DefineRoot(NULL, lowestFreqNode1->GetRoot(), lowestFreqNode2->GetRoot());
		forest.insert(forest.begin(), foo);
		// the order has changed, re-sort
		// FIXME: this is very inefficient, there are better ways
		sort(forest.begin(), forest.end(), __TreeFreqCmp);
	}
}


// using freqTable that was built by PopulateForest(),
// walk through the character list and store the bit code associated with each
inline void THuffman::_BuildBitTable()
{
	//puts("_BuildBitTable()");
	std::map<const char, unsigned long>::iterator itr;
	for(itr = freqTable.begin(); itr != freqTable.end(); itr++) {
		char character = itr->first;
		TBitBuffer *bb = new TBitBuffer;
		bb->AssignBits(forest.at(0)->BitCode(character));
		bitTable.insert(std::make_pair(character, bb));
		//printf("%c = %s\n", character, bb->ReadAllBits().c_str());
	}
}


inline int THuffman::Encode(const std::string & inputFile, const std::string & outputFile)
{
	std::ifstream fInput;
	std::ofstream fOutput;

	fInput.open(inputFile.c_str(), std::ios::in);
	if(!fInput.is_open()) return 1;
	fOutput.open(outputFile.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);

	int r = Encode(fInput, fOutput);

	fInput.close();
	fOutput.close();

	return r;
}
inline int THuffman::Encode(std::ifstream & fInput, std::ofstream & fOutput)
{
	if(!fInput.is_open()) return 1;
	if(!fOutput.is_open()) return 2;

	std::string line, text = "";

	// get file size
	fInput.seekg(0, std::ios::end);
	std::fstream::pos_type totalBytes = fInput.tellg();
	fInput.seekg(0, std::ios::beg);

	if(!totalBytes) return 3;

	while(!fInput.eof()) {
		getline(fInput, line);
		if(fInput.bad()) return 4;
		text.append(line + "\n");
	}
	// in the previous loop we add one too many newlines, removed here
	text.erase(text.size() - 1);

	text.assign(Encode(text));

	fOutput.write(text.c_str(), text.size());
	if(fOutput.bad()) return 5;
	fOutput.flush();

	return 0;
}
inline int THuffman::Decode(const std::string & inputFile, const std::string & outputFile)
{
	std::ifstream fInput;
	std::ofstream fOutput;

	fInput.open(inputFile.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
	if(!fInput.is_open()) return 1;
	fOutput.open(outputFile.c_str(), std::ios::out | std::ios::trunc);

	int r = Decode(fInput, fOutput);

	fInput.close();
	fOutput.close();

	return r;
}
inline int THuffman::Decode(std::ifstream & fInput, std::ofstream & fOutput)
{
	if(!fInput.is_open()) return 1;
	if(!fOutput.is_open()) return 2;

	std::string text;

	// get file size
	fInput.seekg(0, std::ios::end);
	std::fstream::pos_type totalBytes = fInput.tellg();
	fInput.seekg(0, std::ios::beg);

	if(!totalBytes) return 3;

	char * buf = new char[totalBytes];
	fInput.read(buf, totalBytes);
	text.assign(buf, totalBytes);
	delete[] buf;
	if(fInput.bad()) return 4;

	text.assign(Decode(text));

	fOutput << text;
	if(fOutput.bad()) return 5;
	fOutput.flush();

	return 0;
}


// print out the trees in the forest in detail
inline void THuffman::__DebugForest()
{
	puts("_DebugForest()");
	unsigned int forestSize = forest.size();
	for(unsigned int i = 0; i < forestSize; i++)
		forest.at(i)->Describe();
}


// free the memory we allocated
// encoding makes the most mess
inline void THuffman::__CleanUp()
{
	//puts("_CleanUp()");

	// there should only be one item in the forest at this point
	// however, for compleatness, we shall check every element
	// calling DestroyTree() on each
	unsigned int forestSize = forest.size();
	for(unsigned int i = 0; i < forestSize; i++)
		forest.at(i)->DestroyTree();

	std::map<const char, TBitBuffer*>::iterator itr;
	for(itr = bitTable.begin(); itr != bitTable.end(); itr++)
		delete itr->second;

	forest.clear();
	bitTable.clear();
	freqTable.clear();
	codeTable.clear();
}
