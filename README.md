# THuffman - Toby's huffman compression

I originally wrote this in 2006 and I don't remember what drove me to write
this at the time. Something along the lines of I wanted to write something in
c++ and this seemed like a good idea at the time.

Converts plain text to [huffman
encoded](https://en.wikipedia.org/wiki/Huffman_coding) strings and visa-versa.

Limitations: The message header produced is quite large and so currently not
suitable on very short strings with low repetition. The code was never written
to be production worthy, just proof-of-concept.

## Usage

huffman.h can be used in other programs as follows:

```
#include "huffman.h"

[...]

THuffman huff;
string encoded = huff.Encode("peter piper picked a peck of picked peppers");
// probably best not to put binary data to a console, oh well...
puts(encoded.c_str());
// this won't make your console cry though...
puts(huff.Decode(encoded).c_str());

huff.Encode("c:\some\plain.file", "c:\some\encoded.file");
huff.Decode("c:\some\encoded.file", "c:\some\decoded.file");
```

A command-line client is also included, usage:

```
huffman.exe -e|d [input file] [output file]
```
