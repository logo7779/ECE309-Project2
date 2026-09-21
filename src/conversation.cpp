#include "core/conversation.h"
#include <stdexcept>

Conversation::Conversation()    //Constructor
    : data_(nullptr), size_(0), capacity_(0){} //initial constructor array

Conversation::~Conversation(){ //Destructor
    delete[] data_; //Delete array type data
}

Conversation::Conversation(const Conversation& other)   //Copy Constructor
    : data_(nullptr), size_(other.size_), capacity_(other.capacity_){
    if (capacity_ > 0){ //If not empty Conversation
        data_ = new Message[capacity_]; //create new memory
        for (std::size_t i = 0; i < size_; ++i){
            data_[i] = other.data_[i];  //Copy over original to new copy
        }
    }   //Deep-Copy creation here, pointer isnt shared 
}

Conversation& Conversation::operator=(const Conversation& other){   //Copy Assignment operator
    if (this == &other){    //Are the converstaions already the same?
        return *this;   //No extra processing if so
    }
    Message* new_data = nullptr;    //Not the same, create temp data
    if (other.capacity_ > 0){
        new_data = new Message[other.capacity_];    //Copy original to new
        for (std::size_t i = 0; i < other.size_; ++i){
            new_data[i] = other.data_[i];   //Copy original to new
        }
    }
    delete[] data_; //Deallocate original
    //Now re-insert original data into its new spot
    data_ = new_data;   
    size_ = other.size_;
    capacity_ = other.capacity_;

    return *this;
}

Conversation::Conversation(Conversation&& other) noexcept   //Move Constructor
    : data_(other.data_), size_(other.size_), capacity_(other.capacity_){
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
}   //Transfers pointer ownership instead of copying and creating new

Conversation& Conversation::operator=(Conversation&& other) noexcept{   //Move assignment operator
    if (this == &other){    //if already identical dont bother
        return *this;
    }
    delete[] data_; //remove memory
    data_ = other.data_;    //Copy RHS to LHS
    size_ = other.size_;
    capacity_ = other.capacity_;
    other.data_ = nullptr;  //Empty out RHS
    other.size_ = 0;
    other.capacity_ = 0;
    return *this;   //Other is empty now like a newly made Conversation with nothing in it
}

void Conversation::append(Message m){   //Insert new conversation into message history array
    if (size_ == capacity_) {   //If at capacity, allocate more space to dynamic array
        std::size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2;    //Using a growth factor of 2 for ease of implementation
        Message* new_data = new Message[new_capacity];
        for (std::size_t i = 0; i < size_; ++i){
            new_data[i] = std::move(data_[i]);
        }
        delete[] data_;
        data_ = new_data;
        capacity_ = new_capacity;
    }
    data_[size_] = std::move(m);
    ++size_;
}

std::size_t Conversation::size() const noexcept{
    return size_;
}

const Message& Conversation::at(std::size_t i) const{
    if (i >= size_){
        throw std::out_of_range("Conversation index out of range");
    }
    return data_[i];
}

const Message* Conversation::begin() const noexcept{
    return data_;
}

const Message* Conversation::end() const noexcept{
    return data_ == nullptr ? nullptr : data_ + size_;
}