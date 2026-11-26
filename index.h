#pragma once
#include "btree.h"
#include <filesystem>

string CollectionVersion(const string& content);

void CreateIndex(const filesystem::path& path, HashTable& table,
	const string& input, const string& current_version);

bool FindIndexedIds(const filesystem::path& path, HashTable& table,
	const JsonValue& query, HashTable& ids, const string& current_version);