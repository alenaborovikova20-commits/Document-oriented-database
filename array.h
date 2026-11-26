#pragma once
#include <iostream>
#include <string>
#include <stdexcept>
#include <climits>
using namespace std;

template <typename T>
struct Array{
	using Value = T;
	T* data = nullptr;
	int size = 0;

	Array() = default;
	~Array(){delete[] data;}
	Array(const Array& other){
		Array copy;
		if (other.size == 0){return;}

		copy.data = new T[other.size];
		copy.size = other.size;
		for (int i = 0; i < copy.size; ++i){
			copy.data[i] = other.data[i];
		}
		Swap(copy);
	}

	Array(Array&& other) noexcept{Swap(other);}

	Array& operator=(Array other){
		Swap(other);
		return *this;
	}

	void Swap(Array& other) noexcept{
		T* old_data = data;
		int old_size = size;

		data = other.data;
		size = other.size;
		other.data = old_data;
		other.size = old_size;
	}
};

template <typename T>
void CleanArray(Array<T>* arr){
	delete[] arr->data;
	arr->data = nullptr;
	arr->size = 0;
}

template <typename T>
void InitArray(Array<T>* arr){
	CleanArray(arr);
}

template <typename T>
void MPUSH_Index(Array<T>* arr, int index, const typename Array<T>::Value& value){
	if (index < 0 || index > arr->size){return;}
	if (arr->size == INT_MAX){throw length_error("Массив слишком большой");}

	Array<T> new_arr;
	new_arr.size = arr->size + 1;
	new_arr.data = new T[new_arr.size];

	for (int i = 0; i < index; ++i){new_arr.data[i] = arr->data[i];}
	new_arr.data[index] = value;
	for (int i = index; i < arr->size; ++i){new_arr.data[i + 1] = arr->data[i];}

	arr->Swap(new_arr);
}

template <typename T>
void MPUSH(Array<T>* arr, const typename Array<T>::Value& value){
	MPUSH_Index<T>(arr, arr->size, value);
}

template <typename T>
T MGET(Array<T>* arr, int index){
	if (index < 0 || index >= arr->size){return T{};}
	return arr->data[index];
}

template <typename T>
void MDEL(Array<T>* arr, int index){
	if (index < 0 || index >= arr->size){return;}

	Array<T> new_arr;
	new_arr.size = arr->size - 1;
	if (new_arr.size != 0){new_arr.data = new T[new_arr.size];}

	for (int i = 0; i < index; ++i){new_arr.data[i] = arr->data[i];}
	for (int i = index + 1; i < arr->size; ++i){new_arr.data[i - 1] = arr->data[i];}

	arr->Swap(new_arr);
}

template <typename T>
void MSET(Array<T>* arr, int index, const typename Array<T>::Value& value){
	if (index < 0 || index >= arr->size){return;}
	arr->data[index] = value;
}

template <typename T>
int MLEN(Array<T>* arr){
	return arr->size;
}

template <typename T>
void PRINT(Array<T>* arr){
	for (int i = 0; i < arr->size; ++i){cout << arr->data[i] << " ";}
	cout << endl;
}

template <typename T>
void SaveArray(Array<T>* arr, ostream& out){
	out << arr->size << '\n';
	for (int i = 0; i < arr->size; ++i){out << arr->data[i] << " ";}
	out << '\n';
}

template <typename T>
void LoadArray(Array<T>* arr, istream& in){
	int count = 0;
	if (!(in >> count) || count < 0){return;}

	Array<T> loaded;
	for (int i = 0; i < count; ++i){
		T value{};
		if (!(in >> value)){return;}
		MPUSH(&loaded, value);
	}
	arr->Swap(loaded);
}