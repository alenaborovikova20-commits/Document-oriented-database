#pragma once
#include <iostream>
#include <string>
#include <stdexcept>
#include <cstdint>
#include "array.h"

using namespace std;

struct Node{
	string key;
	string value;
	Node* next = nullptr;};

struct HashCell{
	Node* node = nullptr;};

class HashTable{
	public:
		Array<HashCell> cells;
		unsigned int base = 31;
		unsigned int capacity = 0;
		int size = 0;

	HashTable(unsigned int baseNew);
	~HashTable();

	void InitHashTable(unsigned int capacityNew);
	uint64_t Hash(const string& key);
	void Items();
	Node* Get(const string& key);
	void Put(const string& key, const string& value);
	void Remove(const string& key);
	void RebuildTable(unsigned int capacityNew);
};