// huffman_btree.h
/*
	Copyright (c) 2006, Toby Oxborrow <www.oxborrow.net>
	All rights reserved. See LICENCE for details.
	--
	Toby's Huffman Compression - THuffmanBTree
	This class manages a Binary Tree specifically for use with THuffman.
	There is nothing particually remarkable about this BTree. Maybe only
	the BitCode() function to return the path to a character.
*/
#pragma once


class THuffmanBTree {

	public: 

		struct node_t {
			unsigned char letter;
			unsigned int totalFrequency;
			node_t *left;
			node_t *right;
		};

	private:

		void			_Insert(unsigned char, unsigned int, node_t *);
		std::string		_BitCode(unsigned char, node_t *, std::string);
		void			_DestroyTree(node_t *);
		void			_Describe(node_t *, unsigned int);
	    
		node_t			*root;

	public:

		THuffmanBTree() { root=NULL; }
		~THuffmanBTree() { DestroyTree(); }

		void			Insert(unsigned char, unsigned int);
		void			DefineRoot(const unsigned char, node_t *, node_t *);
		std::string		BitCode(unsigned char a) { return _BitCode(a, root, ""); }
		void			Describe() { _Describe(root, 0); }
		void			DestroyTree() { _DestroyTree(root); }

		// Getters and Setters
		node_t			*GetRoot() { return root; }
		unsigned char	GetRootLetter() { return root->letter; }
		unsigned int	GetRootFreq() { return root->totalFrequency; }
		void			SetRootFreq(unsigned int a) { root->totalFrequency = a; }

};


// set up the root node
inline void THuffmanBTree::DefineRoot(const unsigned char _letter, node_t *newLeft, node_t *newRight)
{
	root = new node_t;
	root->letter = _letter;
	root->totalFrequency = newLeft->totalFrequency + newRight->totalFrequency;
	root->left = newLeft;
	root->right = newRight;
}


// return the bitcode of a character by walking the path to it
// this isn't as good as it could be.
inline std::string THuffmanBTree::_BitCode(unsigned char needle, node_t *leaf, std::string bitCode)
{
	if(leaf == NULL) return "";
	if(leaf->letter == needle) return bitCode;
	
	// TODO: this could be improved
	// we create too many string objects
	std::string foo;
	foo = _BitCode(needle, leaf->left, bitCode + "0");
	if(foo.size() > 0) return foo;
	foo = _BitCode(needle, leaf->right, bitCode + "1");
	return foo;
}


// simple insert
inline void THuffmanBTree::Insert(const unsigned char _letter, const unsigned int _totalFrequency)
{
	//printf("Insert(%c, %u)\n", _letter, _totalFrequency);
	if(root != NULL)
		_Insert(_letter, _totalFrequency, root);
	else {
		root = new node_t;
		root->letter = _letter;
		root->totalFrequency = _totalFrequency;
		root->left = NULL;
		root->right = NULL;
	}
}


// simple insert, with leaf node to search from
inline void THuffmanBTree::_Insert(const unsigned char _letter, const unsigned int _totalFrequency, node_t *leaf)
{
	//printf("Insert(%c, %u, NODE)\n", _letter, _totalFrequency);
	if(leaf->totalFrequency > _totalFrequency) {
		if(leaf->left != NULL)
			_Insert(_letter, _totalFrequency, leaf->left);
		else {
			leaf->left = new node_t;		// We can add a new node here
			leaf->left->letter = _letter;
			leaf->left->totalFrequency = _totalFrequency;
			leaf->left->left = NULL;		// Set the children of the new node to null
			leaf->left->right = NULL;
		}
	} else if(leaf->totalFrequency <= _totalFrequency) {
		if(leaf->right != NULL)
			_Insert(_letter, _totalFrequency, leaf->right);
		else {
			leaf->right = new node_t;
			leaf->right->letter = _letter;
			leaf->right->totalFrequency = _totalFrequency;
			leaf->right->left = NULL;
			leaf->right->right = NULL;
		}
	}
}


// with a given leaf node, delete all its children, then itself
inline void THuffmanBTree::_DestroyTree(node_t *leaf)
{
	//printf("DestroyTree(NODE)\n");
	if(leaf == NULL) return;
	_DestroyTree(leaf->left);
	_DestroyTree(leaf->right);
	delete leaf;
}


// just for debug purposes
inline void THuffmanBTree::_Describe(node_t *leaf, unsigned int depth)
{
	if(leaf == NULL) return;

	// draw our depth
	for(unsigned int x = 0; x < depth; x++)
		printf("--=");

	// talk about ourself
	unsigned char tmp = leaf->letter;
	if(tmp == NULL) tmp = '+';
	printf("%c(%u)\n", tmp, leaf->totalFrequency);

	// move on
	if(leaf->left != NULL) _Describe(leaf->left, depth+1);
	if(leaf->right != NULL) _Describe(leaf->right, depth+1);
}
