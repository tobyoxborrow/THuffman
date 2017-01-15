// bit_buffer.h
/*
	Copyright (c) 2006, Toby Oxborrow <www.oxborrow.net>
	All rights reserved. See LICENCE for details.
	--
	Toby's Huffman Compression - TBitBuffer
	A wrapper for a string object to handle a stream of bits.
	
	Usage Example:

	TBitBuffer foo;
	foo.Assign("0000");
	foo.AppendByte(200);		// will be converted to "11001000"
	foo.AppendByte('a');		// will be converted to "10011111"
	foo.AppendBits("00");
	
	puts(foo.ReadAllBits());	// prints 00001100100010011111100
	
	// note, since we are two bits short of three bytes, ReadAllBytes() 
	// will pad the result out by adding two extra 0s
	fstream somefile("c:\some\file");
	somefile << foo.ReadAllBytes();
*/
#pragma once
#include <bitset>
#include <string>


class TBitBuffer {

	private:

		std::string			bitsBuffer;		// the buffer itself
		unsigned long		bufferPos;		// used for Read operations

		// adds one bit to our buffer
		void _AppendBit(const char bit) { bitsBuffer.append(1, bit); }
		// returns the next bit from our buffer
		bool _ReadBit() { return (bitsBuffer.at(bufferPos++) == '1'); }

	public:

		TBitBuffer() { bitsBuffer.clear(); bufferPos = 0; }
		//~TByteBuffer() { }

		void					AssignBits(const std::string &);
		void					AssignBytes(const std::string &);

		void					AppendBits(const std::string &);
		void					AppendNumber(const unsigned long);
		void					AppendByte(const char);
		void					AppendPadding(const unsigned long);

		// when calling these Read functions, a position marker is moved
		// allowing you to read to read the whole stream of bits via
		// successive calls
		std::string				ReadBits(const unsigned int);
		char					ReadBit() { return (_ReadBit() == true) ? '1' : '0'; }
		unsigned long			ReadNumber();
		char					ReadByte();
		void					ReadPadding();

		// these Read functions return the entire contents and do not
		// alter the position marker for the previous Read functions
		std::string				ReadAllBits() { return bitsBuffer; }
		std::string				ReadAllBytes();

		void					Clear() { bitsBuffer.clear(); bufferPos = 0; }

		unsigned long			Size() { return bitsBuffer.size() - bufferPos; }

};


// set the buffer to a string of ones and zeros
inline void TBitBuffer::AssignBits(const std::string & input)
{
	bitsBuffer.assign(input);
	bufferPos = 0;
}
// set the buffer using byte data that must be decomposed first
inline void TBitBuffer::AssignBytes(const std::string & input)
 {
	bitsBuffer.clear();
	bufferPos = 0;
	unsigned char mask = 0x80;
	unsigned int len = input.size();
	for(unsigned int i=0;i<len;i++) {
		char character = input.at(i);
		for(char j=0;j<8;j++) {
			char bit = (character & mask) ? '1' : '0';
			_AppendBit(bit);
			mask = mask >> 1;
		}
		mask = 0x80;
	}
}


// the 'input' string should be some binary value to add to our buffer
// example: 01000101
inline void TBitBuffer::AppendBits(const std::string & input)
{
	bitsBuffer.append(input);
}
// returns as many bits as 'len'
inline std::string TBitBuffer::ReadBits(const unsigned int len) 
{
	if(len == 1)
		return (_ReadBit() == true) ? "1" : "0";

	std::string r = "";
	for(unsigned int i=0;i<len;i++) {
		char bit = (_ReadBit() == true) ? '1' : '0';
		r.append(1, bit);
	}
	return r;
}


/*
	writes a number in a compact tally form. the aim is to stay <= 8 bits
	values between 1 and 25 will remain <= 8 bits (although some do go over)
	[multiples of 5]0[remainder]0
	1 is a tally
	0 is a terminator
	example:
		    12345678
		0 = 00
		1 = 010
		2 = 0110
		3 = 01110
		4 = 011110
		5 = 100
		etc.
	for numbers that are expected to be > 25, use a normal numeric value with AppendByte
*/
inline void TBitBuffer::AppendNumber(const unsigned long value)
{
	unsigned int a = value / 5;
	unsigned int b = value % 5;
	for(unsigned int i=0;i<a;i++)
		_AppendBit('1');
	_AppendBit('0');
	for(unsigned int i=0;i<b;i++)
		_AppendBit('1');
	_AppendBit('0');
}
// returns a number made up by AppendNumber()
inline unsigned long TBitBuffer::ReadNumber()
{
	unsigned int a = 0, b = 0;
	while(_ReadBit() != false)
		a++;
	while(_ReadBit() != false)
		b++;
	return (a*5)+b;
}


// add some bits to pad the total stream so it is aligned to 8 bits
// padding is made up of 0's then a 1 to mark the end
inline void TBitBuffer::AppendPadding(const unsigned long totalSize)
{
	unsigned int paddingSize = totalSize % 8;
	paddingSize = 8 - paddingSize;
	if(paddingSize < 8) {
		paddingSize--;		// leave space for the '1'
		for(unsigned char i=0;i<paddingSize;i++)
			_AppendBit('0');
	} else {
		/*
		- !! worst case scenario !! -
		the block doesn't require padding! gagh!
		but we *must* add the '1' bit to mark the end of padding
		thus we end up writing a *byte*, just to store the *1 bit* :(
		*/
		for(unsigned char i=0;i<7;i++)
			_AppendBit('0');
	}
	_AppendBit('1');		// end of padding marker
}
// this just moves the pointer past the padding
inline void TBitBuffer::ReadPadding()
{
	for(unsigned char i=0;i<8;i++)
		if(_ReadBit() == true) break;
}


inline void TBitBuffer::AppendByte(const char input)
{
	unsigned char mask = 0x80;		// 10000000
	//for(int i=7;i>=0;i--) {
	for(int i=0;i<8;i++) {
		char bit = (input & mask) ? '1' : '0';
		_AppendBit(bit);
		mask = mask >> 1;			// 01000000 and so on
	}
}
// returns a single byte
inline char TBitBuffer::ReadByte()
{
	// get a byte
	std::bitset<8> byte;
	for(int i=0;i<8;i++)
		byte.set((7-i), _ReadBit());
	return (char)byte.to_ulong();
}


// translate a string of binary to real binary bytes
// will be padded up to nearest byte if too short
inline std::string TBitBuffer::ReadAllBytes()
{
	unsigned long bufferSize = bitsBuffer.size();
	std::string bytes = "";
	std::bitset<8> byte;
	// move through the string in byte chunks
	for(unsigned int i=0;i<bufferSize;i+=8) {
		byte.reset();
		for(unsigned char j=0;j<8;j++) {
			if((i+j) >= bufferSize) break;
			byte.set((7-j), (bitsBuffer.at(i+j) == '1'));
		}
		bytes.append(1, (char)byte.to_ulong());
	}
	return bytes;
}
