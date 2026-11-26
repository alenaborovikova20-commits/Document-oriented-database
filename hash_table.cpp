#include "hash_table.h"
using namespace std;



HashTable::HashTable(unsigned int baseNew) : base(baseNew){}

void HashTable::InitHashTable(unsigned int capacityNew){
	if (capacityNew == 0 || cells != nullptr) {return;}
	
	cells = new HashCell[capacityNew];
	capacity = capacityNew;
}

// полиномиальный хэш
uint64_t HashTable::Hash(const string& key){
	if (capacity == 0){
		throw std::logic_error("Хэш-таблица не инициализирована");
	}

	uint64_t hash = 0;
	for (unsigned char item : key){
		hash = hash * base + item;
	}
	return hash % capacity;}


void HashTable::Items(){
	for (unsigned int i = 0; i < capacity; i++){
		Node* cur = cells[i].node;
		while(cur != nullptr){
			cout << cur->key << " " << cur->value << endl;
			cur = cur->next;
		}

}}

Node* HashTable::Get(const string& key){
	Node* cur = cells[Hash(key)].node;
	while(cur != nullptr){
		if (cur->key == key){
			return cur;
		}
		cur = cur->next;}
return nullptr;
}


void HashTable::Put(const string& key, const string& value){
	uint64_t index = Hash(key);
	if (cells[index].node != nullptr){
		Node* cur = cells[index].node;
		while (cur->next != nullptr){
			if (cur->key == key) {
				cur->value = value;
				return;}
			cur = cur->next;
		}
		if (cur->key == key) {
				cur->value = value;
				return;}
		cur->next = new Node{key, value, nullptr};
	}else{
		cells[index].node = new Node{key, value, nullptr};	
	}
	size++;
	if ((double)size / capacity >= 0.75){RebuildTable(capacity * 2);}
}


void HashTable::Remove(const string& key){
	uint64_t index = Hash(key);
	Node* cur = cells[index].node;
	if (cur == nullptr) {return;}

	if (cur->key == key) {
		cells[index].node = cur->next;
		delete cur;
		--size;
		return;}


	while (cur->next!= nullptr && cur->next->key != key){cur = cur->next;}
	if(cur->next == nullptr) {return;}
	Node* delNode = cur->next;
	cur->next = delNode->next;
	delete delNode;
	--size;

}
void HashTable::RebuildTable(unsigned int capacity){
	if (capacity == 0) {return;}
	HashCell* newCells = new HashCell[capacity];
	HashCell* oldCells = cells;
	unsigned int oldCapacity = this->capacity;
	
	cells = newCells;
	this->capacity = capacity;

	for (unsigned int i = 0; i < oldCapacity; ++i) {
		Node* cur = oldCells[i].node;
		while (cur != nullptr){
			Node* next = cur->next;
			uint64_t index = Hash(cur->key);
			cur->next = cells[index].node;
			cells[index].node = cur;
			cur = next;}
	}
	delete[] oldCells;
}


HashTable::~HashTable() {
	for (unsigned int i = 0; i < capacity; ++i) {
		Node* cur = cells[i].node;

		while (cur != nullptr) {
			Node* next = cur->next;
			delete cur;
			cur = next;
		}
	}
	delete[] cells;
}
