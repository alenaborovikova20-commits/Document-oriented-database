#pragma once
#include "btree.h"
#include <filesystem>

void CreateIndex(const filesystem::path& path, HashTable& table, const string& input);
bool FindIndexedIds(const filesystem::path& path, HashTable& table, const JsonValue& query, HashTable& ids);
