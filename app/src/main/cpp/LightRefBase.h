//
// Created by 余均宇 on 2019-07-05.
//

#ifndef SMARTTOOLS_LIGHTREFBASE_H
#define SMARTTOOLS_LIGHTREFBASE_H

#include <stdio.h>
#include <atomic>
#include <Define.h>

using namespace std;

template <class T>
class LightRefBase {
public:
    LightRefBase():mCount(0){

    }

    void incStrong(){
        mCount.fetch_add(1,memory_order_relaxed);
        LOGE("inc");
    }

    void decStrong(){
        LOGE("dec");
        if(mCount.fetch_sub(1,memory_order_release) == 1){
            atomic_thread_fence(memory_order_acquire);
            delete static_cast<const T*>(this);
            LOGE("delete");
        }
    }


private:
    mutable atomic<int> mCount;

};


#endif //SMARTTOOLS_LIGHTREFBASE_H
