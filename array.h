#pragma once
#include <iostream>
#include <string>
#include <stdexcept>
#include <climits>
using namespace std;

template <typename T>
struct Array{
	T* data = nullptr;
	int size = 0;

	Array() = default;
	~Array(){delete[] data;}

	Array(const Array& other){
		if (other.size == 0){return;}

		Array copy;
		copy.data = new T[other.size];
		copy.size = other.size;
		for (int i = 0; i < copy.size; ++i){
			copy.data[i] = other.data[i];
		}
		Swap(copy);
	}

	Array& operator=(const Array& other){
		if (this == &other){return *this;}

		Array copy(other);
		Swap(copy);
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
void InitArray(Array<T>* arr, int size = 0){
	if (size < 0){throw length_error("Некорректный размер массива");}

	Array<T> new_arr;
	if (size > 0){
		new_arr.data = new T[size]{};
		new_arr.size = size;
	}

	arr->Swap(new_arr);
}

template <typename T>
void MPUSH_Index(Array<T>* arr, int index, const T& value){
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
void MPUSH(Array<T>* arr, const T& value){
	MPUSH_Index(arr, arr->size, value);
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
void MSET(Array<T>* arr, int index, const T& value){
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

