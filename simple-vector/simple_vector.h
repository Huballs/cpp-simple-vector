#pragma once

#include <cassert>
#include <initializer_list>
#include <stdexcept>
#include <algorithm>
#include "array_ptr.h"

class ReserveProxyObj{
    public:
    ReserveProxyObj(size_t capacity_to_reserve) : capacity_(capacity_to_reserve){}
    size_t capacity_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}


template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    explicit SimpleVector(size_t size)
        : array_(size), size_(size), capacity_(size) {

        std::generate(begin(), end(), [](){return Type();});
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value)
        : array_(size), size_(size), capacity_(size)  {
    
        std::fill(begin(), end(), value);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) 
    : array_(init.size()), size_(init.size()), capacity_(init.size()) {

        std::move(init.begin(), init.end(), begin());
    }

    SimpleVector(const SimpleVector& other) {
        SimpleVector buffer(other.size_);
        std::copy(other.begin(), other.end(), buffer.begin());
        swap(buffer);
    }

    SimpleVector(SimpleVector&& other) {
        Resize(other.size_);
        std::move(other.begin(), other.end(), begin());
        other.size_ = 0;
        other.capacity_ = 0;
    }

    SimpleVector(ReserveProxyObj reserveObj){
        Reserve(reserveObj.capacity_);
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if(*this != rhs) {
            SimpleVector tmp(rhs);
            swap(tmp);
        }
        return *this;
    }
    SimpleVector& operator=( SimpleVector&& rhs) {
        if(*this != rhs) {
            swap(rhs);
        }
        return *this;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        return array_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        return array_[index];
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return !(bool)size_;
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if(index >= GetSize())
            throw std::out_of_range("Too High");
        return array_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if(index >= GetSize())
            throw std::out_of_range("Too High");
        return array_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if(new_size > GetSize() && new_size > GetCapacity()){
            ArrayPtr<Type> tmp_array(new_size);
            std::move(begin(), end(), tmp_array.Get());
            array_.swap(tmp_array);
            std::generate(begin()+size_, begin()+new_size, [](){return Type();});
            size_ = new_size;
            capacity_ = new_size;

        } else if (new_size > GetSize() && new_size <= capacity_){
            std::generate(begin()+size_, begin()+new_size, [](){return Type();});
            size_ = new_size;

        } else if (new_size < GetSize()){
            size_ = new_size;
        }
    }

    void Reserve(size_t new_capacity){
        if (new_capacity > GetCapacity()){
            ArrayPtr<Type> tmp_array(new_capacity);
            std::move(begin(), end(), tmp_array.Get());
            array_.swap(tmp_array);
            capacity_ = new_capacity;
        }
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack( Type&& item) {
        Insert(end(), std::move(item));
    }
    void PushBack(const Type& item) {
        Type item_ = item;
        Insert(end(), std::move(item_));
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos,  Type&& value) {
        size_t size = GetSize();
        int pos_ = pos - begin();
        if(size >= GetCapacity()){
            Resize(size ? size*2 : 1);
            size_ = size + 1;
            std::move_backward(begin() + pos_, end()-1, end());
            *(begin() + pos_) = std::move(value);
            
        } else {
            std::move_backward(begin() + pos_, end(), end() + 1);
            *(begin() + pos_) = std::move(value);
            ++size_;
        }
        return begin() + pos_;
    }
    Iterator Insert(ConstIterator pos, const Type& value) {
        Type value_ = value;
        return Insert(pos, std::move(value_));
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        if(!IsEmpty())
            --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        int pos_ = pos - begin();
        std::move(begin() + pos_ + 1, end(), begin() + pos_);
        --size_;
        return begin() + pos_;
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        array_.swap(other.array_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return array_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return &array_[GetSize()];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return array_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return &array_[GetSize()];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return array_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return &array_[GetSize()];
    }

    private:

    ArrayPtr<Type> array_;
    size_t size_ = 0;
    size_t capacity_ = 0;

};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    if(lhs.GetSize() != rhs.GetSize()) return false;
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs==rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs > lhs);
} 

template <typename Type>
std::ostream& operator<<(std::ostream& stream, const SimpleVector<Type>& array){
    for(const Type item : array){
        stream << item << ",";
    }
    return stream;
}

