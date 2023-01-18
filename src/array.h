#pragma once

#include "general.h"

#include <stdlib.h>
#include <string.h>

template <typename Array>
struct Array_Iterator {
    using Value_Type = typename Array::Value_Type;
    using Pointer_Type = Value_Type *;
    using Reference_Type = Value_Type &;
    
    inline Array_Iterator(Pointer_Type ptr) {
        this->ptr = ptr;
    }

    inline Array_Iterator &operator++() {
        ptr++;
        return *this;
    }
    
    inline Array_Iterator operator++(int) {
        Array_Iterator iterator = *this;
        ++(*this);
        return iterator;
    }
    
    inline Array_Iterator &operator--() {
        ptr--;
        return *this;
    }
    
    inline Array_Iterator operator--(int) {
        Array_Iterator iterator = *this;
        --(*this);
        return iterator;        
    }

    inline Reference_Type operator[](int index) {
        return *(ptr + index);
    }
    
    inline Pointer_Type operator->() {
        return ptr;
    }
    
    inline Reference_Type operator*() {
        return *ptr;
    }

    inline bool operator==(Array_Iterator const &other) const {
        return ptr == other.ptr;
    }
    
    inline bool operator!=(Array_Iterator const &other) const {
        return !(*this == other);
    }
    
    Pointer_Type ptr;
};

template <typename T>
struct Array {
    using Value_Type = T;
    using Iterator = Array_Iterator<Array<T>>;
    
    T *data = NULL;
    int allocated = 0;
    int count = 0;
    bool use_temporary_storage = false;

    ~Array();

    void reserve(int size);
    void resize(int size);
    void add(T const &item);
    T *add();
    int find(T const &item);
    void ordered_remove_by_index(int n);

    T const &operator[](int index) const;
    T &operator[](int index);

    inline Iterator begin() { return data; }
    inline Iterator end() { return data + count; }
};

template <typename T>
inline Array <T>::~Array() {
    if (data && !use_temporary_storage) {
        free(data);
    }
}

template <typename T>
inline void Array <T>::reserve(int size) {
    if (allocated >= size) return;

    int new_allocated = Max(allocated, size);
    new_allocated = Max(new_allocated, 32);

    int new_bytes = new_allocated * sizeof(T);
    int old_bytes = allocated * sizeof(T);

    void *new_data = use_temporary_storage ? talloc(new_bytes) : malloc(new_bytes);
    if (data) {
        memcpy(new_data, data, old_bytes);
        if (!use_temporary_storage) {
            free(data);
        }
    }

    data = (T *)new_data;
    allocated = new_allocated;
}

template <typename T>
inline void Array <T>::resize(int size) {
    reserve(size);
    count = size;
}

template <typename T>
inline void Array <T>::add(T const &item) {
    reserve(count+1);
    data[count] = item;
    count++;
}

template <typename T>
inline T *Array <T>::add() {
    add({});
    return &data[count-1];
}

template <typename T>
inline int Array <T>::find(T const &item) {
    for (int i = 0; i < count; i++) {
        if (data[i] == item) return i;
    }
    return -1;
}

template <typename T>
inline void Array <T>::ordered_remove_by_index(int n) {
    for (int i = n; i < count-1; i++) {
        data[i] = data[i+1];
    }
    count--;
}

template <typename T>
inline T const &Array <T>::operator[](int index) const {
    assert(index >= 0);
    assert(index < count);
    return data[index];
}

template <typename T>
inline T &Array <T>::operator[](int index) {
    assert(index >= 0);
    assert(index < count);
    return data[index];    
}
