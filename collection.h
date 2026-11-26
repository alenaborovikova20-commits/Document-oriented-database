#pragma once
#include <filesystem>
#include "hash_table.h"


void SaveCollection(const filesystem::path&  path, const HashTable& table);