#pragma once
#include "array.h"
#include "json_parser.h"
#include "hash_table.h"

bool IsIndexKey(const JsonValue& value);

struct IndexEntry{
	JsonValue key;
	Array<string> ids;

	void Add(const string& id);};

struct BTreeNode{
	Array<IndexEntry*> entries;
	Array<BTreeNode*> children;
	int size = 0;

	BTreeNode(int degree, bool leaf);
	~BTreeNode();};

class BTree{
	public:
		int degree;
		BTreeNode* root = nullptr;

		BTree(int degree);
		~BTree();
		BTree(const BTree&) = delete;
		BTree& operator=(const BTree&) = delete;

		void Put(const JsonValue& key, const string& id);
		void Find(const JsonValue& key, HashTable& ids) const;
		void Range(const JsonValue& key, bool greater, HashTable& ids) const;
		void Save(ostream& file) const;
		void Load(istream& file);

	private:
		IndexEntry* Get(const JsonValue& key) const;
		void Split(BTreeNode* parent, int index);
		void Insert(BTreeNode* node, const JsonValue& key, const string& id);
		BTreeNode* LoadNode(istream& file, int depth, int& leaf_depth, const JsonValue* lower, const JsonValue* upper);
};