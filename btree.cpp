#include "btree.h"
#include <stdexcept>

bool IsIndexKey(const JsonValue& value){return value.type == JsonType::Number || value.type == JsonType::String || value.type == JsonType::Bool || value.type == JsonType::Null;}

 void SkipIndexSpace(istream& file){
	int c = file.peek();

	while (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v'){
		file.get();
		c = file.peek();
	}
}

void WriteIndexString(ostream& file, const string& value){
	file.put('"');

	for (char c : value){
		if (c == '"' || c == '\\'){file.put('\\');}
		file.put(c);
	}

	file.put('"');
}

bool ReadIndexString(istream& file, string& value){
	SkipIndexSpace(file);

	char c;
	if (!file.get(c) || c != '"'){return false;}

	value.clear();
	while (file.get(c)){
		if (c == '"'){return true;}
		if (c == '\\' && !file.get(c)){return false;}
		value += c;
	}

	return false;
}



 int CompareKeys(const JsonValue& left, const JsonValue& right){
	if (left.type != right.type){return left.type < right.type ? -1 : 1;}

	switch (left.type){
		case JsonType::String: return left.text.compare(right.text);
		case JsonType::Number: return (left.number > right.number) - (left.number < right.number);
		case JsonType::Bool: return left.boolean - right.boolean;
		default: return 0;
	}
}

void IndexEntry::Add(const string& id){MPUSH(&ids, id);}

BTreeNode::BTreeNode(int degree, bool leaf){
	entries.data = new IndexEntry*[2 * degree - 1]{};
	entries.size = 2 * degree - 1;

	if (!leaf){
		children.data = new BTreeNode*[2 * degree]{};
		children.size = 2 * degree;
	}
}

BTreeNode::~BTreeNode(){
	for (int i = 0; i < size; ++i){
		delete entries.data[i];
	}

	if (children.data != nullptr){
		for (int i = 0; i <= size; ++i){
			delete children.data[i];
		}
	}
}

BTree::BTree(int degree) : degree(degree){
	if (degree < 2 || degree > 64){
		throw runtime_error("Степень B-дерева должна быть от 2 до 64");
	}
}

BTree::~BTree(){
	delete root;
}

 int LowerBound(const BTreeNode* node, const JsonValue& key){
	int i = 0;
	while (i < node->size && CompareKeys(node->entries.data[i]->key, key) < 0){
		++i;
	}
	return i;
}

IndexEntry* BTree::Get(const JsonValue& key) const{
	const BTreeNode* node = root;

	while (node != nullptr){
		int i = LowerBound(node, key);
		if (i < node->size && CompareKeys(node->entries.data[i]->key, key) == 0){
			return node->entries.data[i];
		}
		if (node->children.data == nullptr){return nullptr;}
		node = node->children.data[i];
	}

	return nullptr;
}

void BTree::Split(BTreeNode* parent, int index){
	BTreeNode* left = parent->children.data[index];
	BTreeNode* right = new BTreeNode(degree, left->children.data == nullptr);
	IndexEntry* middle = left->entries.data[degree - 1];

	for (int i = 0; i < degree - 1; ++i){
		right->entries.data[i] = left->entries.data[i + degree];
		left->entries.data[i + degree] = nullptr;
	}

	if (right->children.data != nullptr){
		for (int i = 0; i < degree; ++i){
			right->children.data[i] = left->children.data[i + degree];
			left->children.data[i + degree] = nullptr;
		}
	}

	left->entries.data[degree - 1] = nullptr;
	left->size = degree - 1;
	right->size = degree - 1;

	for (int i = parent->size; i > index; --i){
		parent->entries.data[i] = parent->entries.data[i - 1];
		parent->children.data[i + 1] = parent->children.data[i];
	}

	parent->entries.data[index] = middle;
	parent->children.data[index + 1] = right;
	++parent->size;
}

void BTree::Insert(BTreeNode* node, const JsonValue& key, const string& id){
	int pos = LowerBound(node, key);

	if (node->children.data == nullptr){
		IndexEntry* entry = new IndexEntry;
		try{
			entry->key = key;
			entry->Add(id);
		}catch (...){
			delete entry;
			throw;
		}

		for (int i = node->size; i > pos; --i){
			node->entries.data[i] = node->entries.data[i - 1];
		}

		node->entries.data[pos] = entry;
		++node->size;
		return;
	}

	BTreeNode* child = node->children.data[pos];
	if (child->size == child->entries.size){
		Split(node, pos);
		if (CompareKeys(key, node->entries.data[pos]->key) > 0){++pos;}
	}

	Insert(node->children.data[pos], key, id);
}

void BTree::Put(const JsonValue& key, const string& id){
	if (!IsIndexKey(key)){
		throw runtime_error("Недопустимый тип ключа индекса");
	}

	IndexEntry* entry = Get(key);
	if (entry != nullptr){
		entry->Add(id);
		return;
	}

	if (root == nullptr){root = new BTreeNode(degree, true);}

	if (root->size == root->entries.size){
		BTreeNode* parent = new BTreeNode(degree, false);
		parent->children.data[0] = root;

		try{
			Split(parent, 0);
		}catch (...){
			parent->children.data[0] = nullptr;
			delete parent;
			throw;
		}

		root = parent;
	}

	Insert(root, key, id);
}

 void CollectIds(const IndexEntry* entry, HashTable& ids){
	for (int i = 0; i < entry->ids.size; ++i){
		ids.Put(entry->ids.data[i], "");
	}
}

void BTree::Find(const JsonValue& key, HashTable& ids) const{
	if (!IsIndexKey(key)){return;}

	const IndexEntry* entry = Get(key);
	if (entry != nullptr){CollectIds(entry, ids);}
}

 void CollectRange(const BTreeNode* node, const JsonValue& key,
	bool greater, HashTable& ids){
	int i = greater ? LowerBound(node, key) : 0;

	for (; i <= node->size; ++i){
		if (node->children.data != nullptr){
			CollectRange(node->children.data[i], key, greater, ids);
		}
		if (i == node->size){break;}

		const IndexEntry* entry = node->entries.data[i];
		int cmp = CompareKeys(entry->key, key);
		if (!greater && cmp >= 0){break;}

		if (entry->key.type == key.type && (greater ? cmp > 0 : cmp < 0)){
			CollectIds(entry, ids);
		}
	}
}

void BTree::Range(const JsonValue& key, bool greater, HashTable& ids) const{
	if (root != nullptr && IsIndexKey(key)){
		CollectRange(root, key, greater, ids);
	}
}

 void SaveNode(ostream& file, const BTreeNode* node){
	bool leaf = node->children.data == nullptr;
	file << node->size << ' ' << leaf << '\n';

	for (int i = 0; i < node->size; ++i){
				const IndexEntry* entry = node->entries.data[i];

		WriteIndexString(file, ToJson(entry->key));
		file << ' ' << entry->ids.size;

		for (int j = 0; j < entry->ids.size; ++j){
			file << ' ';
			WriteIndexString(file, entry->ids.data[j]);
		}

		file << '\n';
	}

	if (!leaf){
		for (int i = 0; i <= node->size; ++i){
			SaveNode(file, node->children.data[i]);
		}
	}
}

void BTree::Save(ostream& file) const{
	if (root == nullptr){
		file << "0 1\n";
		return;
	}

	SaveNode(file, root);
}



BTreeNode* BTree::LoadNode(istream& file, int depth, int& leaf_depth,
	const JsonValue* lower, const JsonValue* upper){
	int size = 0;
	int leaf = 0;

	if (!(file >> size >> leaf) || depth > 64 ||size < 0 || size > 2 * degree - 1 || (leaf != 0 && leaf != 1) || (depth > 0 && size < degree -1) || (leaf == 0 && size == 0)){
		throw runtime_error("Некорректный узел индекса");
	}

	if (leaf == 1){
		if (leaf_depth == -1){leaf_depth = depth;}
		if (leaf_depth != depth){
			throw runtime_error("Листья индекса на разной глубине");
		}
	}

	BTreeNode* node = new BTreeNode(degree, leaf == 1);

	try{
		for (int i = 0; i < size; ++i){
			string key_text;
			int count = 0;

			if (!ReadIndexString(file, key_text) || !(file >> count) || count <= 0){ throw runtime_error("Некорректная запись индекса");}
			IndexEntry* entry = new IndexEntry;
			node->entries.data[node->size++] = entry;

			JsonParser parser(key_text);
			entry->key = parser.Parse();

			if (!IsIndexKey(entry->key) || (lower != nullptr && CompareKeys(*lower, entry->key) >= 0) || (upper != nullptr && CompareKeys(entry->key, *upper) >= 0) || (i > 0 && CompareKeys(node->entries.data[i - 1]->key, entry->key) >= 0)){throw runtime_error("Нарушен порядок ключей индекса");}

			for (int j = 0; j < count; ++j){
				string id;
				if (!ReadIndexString(file, id)){throw runtime_error("Не удалось прочитать _id из индекса");}
				entry->Add(id);
			}
		}

		if (node->children.data != nullptr){
			for (int i = 0; i <= size; ++i){
				const JsonValue* child_lower = i == 0 ? lower : &node->entries.data[i - 1]->key;
				const JsonValue* child_upper = i == size ? upper : &node->entries.data[i]->key;

				node->children.data[i] = LoadNode(file, depth + 1, leaf_depth,
					child_lower, child_upper);
			}
		}
	}catch (...){
		delete node;
		throw;
	}

	return node;
}

void BTree::Load(istream& file){
	int leaf_depth = -1;
	BTreeNode* loaded = LoadNode(file, 0, leaf_depth, nullptr, nullptr);

	file >> ws;
	if (!file.eof() || file.bad()){
		delete loaded;
		throw runtime_error("Лишние данные в индексе");
	}

	delete root;
	root = loaded;
}