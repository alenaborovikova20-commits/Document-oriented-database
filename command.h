#pragma once
#include "hash_table.h"
#include <string>
#include <filesystem>

using namespace std;

int ExecuteCommand(const string& command, const string& input, HashTable& table, const filesystem::path& collectionPath);