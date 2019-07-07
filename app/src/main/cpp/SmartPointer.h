//
// Created by 余均宇 on 2019-06-30.
//

#ifndef SMARTTOOLS_SMARTPOINTER_H
#define SMARTTOOLS_SMARTPOINTER_H

#include <stdio.h>

template <class  T>
class SmartPointer {
private:
    T* m_ptr;

public:
    SmartPointer():m_ptr(0){

    }


    SmartPointer(T *ptr){
        if(m_ptr){
            m_ptr = ptr;
            m_ptr->incStrong();
        }


    }


    ~SmartPointer(){
        if(m_ptr){
            m_ptr->decStrong();
        }
    }

    SmartPointer& operator = (T* other){
        if(other){
            m_ptr = other;
            other->incStrong();
        }

        return *this;
    }




};


#endif //SMARTTOOLS_SMARTPOINTER_H
