#include "hash_table.h"
#include <climits>

using namespace std;

HashTable::HashTable(unsigned int baseNew) : base(baseNew){}

void HashTable::InitHashTable(unsigned int capacityNew){
	if (capacityNew == 0 || cells.data != nullptr){return;}
	if (capacityNew > INT_MAX){throw length_error("Хеш-таблица слишком большая");}

	InitArray(&cells, capacityNew);
	capacity = capacityNew;
}

uint64_t HashTable::Hash(const string& key){
	if (capacity == 0){
		throw logic_error("Хеш-таблица не инициализирована");
	}

	uint64_t hash = 0;
	for (unsigned char item : key){
		hash = hash * base + item;
	}
	return hash % capacity;
}

void HashTable::Items(){
	for (unsigned int i = 0; i < capacity; ++i){
		Node* cur = cells.data[i].node;
		while (cur != nullptr){
			cout << cur->key << " " << cur->value << endl;
			cur = cur->next;
		}
	}
}

Node* HashTable::Get(const string& key){
	Node* cur = cells.data[Hash(key)].node;
	while (cur != nullptr){
		if (cur->key == key){return cur;}
		cur = cur->next;
	}
	return nullptr;
}

void HashTable::Put(const string& key, const string& value){
	uint64_t index = Hash(key);

	if (cells.data[index].node != nullptr){
		Node* cur = cells.data[index].node;

		while (cur->next != nullptr){
			if (cur->key == key){
				cur->value = value;
				return;
			}
			cur = cur->next;
		}

		if (cur->key == key){
			cur->value = value;
			return;
		}

		cur->next = new Node{key, value, nullptr};
	}else{
		cells.data[index].node = new Node{key, value, nullptr};
	}

	++size;
	if (size * 4ULL >= capacity * 3ULL && capacity <= INT_MAX / 2){
		RebuildTable(capacity * 2);
	}
}

void HashTable::Remove(const string& key){
	uint64_t index = Hash(key);
	Node* cur = cells.data[index].node;
	if (cur == nullptr){return;}

	if (cur->key == key){
		cells.data[index].node = cur->next;
		delete cur;
		--size;
		return;
	}

	while (cur->next != nullptr && cur->next->key != key){
		cur = cur->next;
	}
	if (cur->next == nullptr){return;}

	Node* delNode = cur->next;
	cur->next = delNode->next;
	delete delNode;
	--size;
}

void HashTable::RebuildTable(unsigned int capacityNew){
	if (capacityNew == 0){return;}
	if (capacityNew > INT_MAX){throw length_error("Хеш-таблица слишком большая");}

	Array<HashCell> new_cells;
	InitArray(&new_cells, capacityNew);
	capacity = capacityNew;

	for (int i = 0; i < cells.size; ++i){
		Node* cur = cells.data[i].node;

		while (cur != nullptr){
			Node* next = cur->next;
			uint64_t index = Hash(cur->key);

			cur->next = new_cells.data[index].node;
			new_cells.data[index].node = cur;
			cur = next;
		}
	}

	cells.Swap(new_cells);
}

HashTable::~HashTable(){
	for (unsigned int i = 0; i < capacity; ++i){
		Node* cur = cells.data[i].node;

		while (cur != nullptr){
			Node* next = cur->next;
			delete cur;
			cur = next;
		}
	}
}