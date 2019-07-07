//
// Created by 余均宇 on 2019-07-05.
//

#include "RefBase.h"
#include <stdio.h>
#include <atomic>


#define INITIAL_STRONG_VALUE (1<<28)

#include <Define.h>



class RefBase::weakref_impl : public RefBase::weakref_type{
public:
    //强引用计数
    std::atomic<int32_t> mStrong;
    //弱引用计数
    std::atomic<int32_t> mWeak;

    //持有计数基础
    RefBase* const mBase;

    //声明周期的标志位
    std::atomic<int32_t> mFlags;


public:
    explicit weakref_impl(RefBase* base)
    :mStrong(INITIAL_STRONG_VALUE),mWeak(0),mBase(base){

    }

};

void RefBase::incStrong(const void *id) const {
    LOGE("inc Strong");
    weakref_impl* const refs = mRefs;
    refs->incWeak(id);
    const int32_t c = refs->mStrong.fetch_add(1,std::memory_order_relaxed);
    //说明不是第一次声明
    if(c != INITIAL_STRONG_VALUE){
        return;
    }

    int32_t old __unused = refs->mStrong.fetch_sub(INITIAL_STRONG_VALUE,std::memory_order_relaxed);

    refs->mBase->onFirstRef();
}

void RefBase::decStrong(const void *id) const {
    LOGE("decStrong");
    weakref_impl* const  refs = mRefs;
    const int32_t c = refs->mStrong.fetch_sub(1,std::memory_order_release);

    if(c == 1){
        std::atomic_thread_fence(std::memory_order_acquire);
        refs->mBase->onLastStrongRef(id);
        int32_t flags = refs->mFlags.load(std::memory_order_relaxed);
        if((flags&OBJECT_LIFETIME_WEAK) == OBJECT_LIFETIME_STRONG){
            LOGE("delete object");
            delete this;
        }
    }

    refs->decWeak(id);
}

int32_t RefBase::getStrongCount(const void *id) const {
    return mRefs->mStrong.load(std::memory_order_relaxed);
}

void RefBase::forceIncStrong(const void *id) const {
    //强制加1
    weakref_impl* const  refs = mRefs;
    refs->incWeak(id);

    const int32_t c = refs->mStrong.fetch_add(1,std::memory_order_relaxed);

    //因为可能出现初始值是INITIAL_STRONG_VALUE ，因此之后需要直接减掉
    switch (c){
        case INITIAL_STRONG_VALUE:
            refs->mStrong.fetch_sub(INITIAL_STRONG_VALUE,
                    std::memory_order_relaxed);
        case 0:
            refs->mBase->onFirstRef();
    }
}

RefBase::weakref_type *RefBase::createWeak(const void *id) const {
    mRefs->incWeak(id);
    return mRefs;
}

RefBase::weakref_type *RefBase::getWeakRef() const {
    return mRefs;
}

RefBase::RefBase():mRefs(new weakref_impl(this)) {

}

RefBase::~RefBase() {
    int32_t flags = mRefs->mFlags.load(std::memory_order_relaxed);

    if((flags&OBJECT_LIFETIME_MASK) ==OBJECT_LIFETIME_STRONG){
        if(mRefs->mWeak.load(std::memory_order_relaxed)){
            delete mRefs;
        }
    } else if(mRefs->mStrong.load(std::memory_order_relaxed)
    == INITIAL_STRONG_VALUE){
        delete mRefs;
    }

    const_cast<weakref_impl*&>(mRefs) = NULL;
}

void RefBase::extendObjectLifetime(int32_t mode) {
    mRefs->mFlags.fetch_or(mode, std::memory_order_relaxed);
}

void RefBase::onFirstRef() {

}

void RefBase::onLastStrongRef(const void *id) {

}

bool RefBase::onIncStrongAttempted(int32_t flags, const void *id) {
    return (flags&FIRST_INC_STRONG) ? true : false;
}

void RefBase::onLastWeakRef(const void *id) {

}

RefBase *RefBase::weakref_type::refBase() const {
    return static_cast<const weakref_impl*>(this)->mBase;
}

void RefBase::weakref_type::incWeak(const void *id) {
    LOGE("incWeak");
    weakref_impl* const impl = static_cast<weakref_impl*>(this);
    const int32_t c __unused = impl->mWeak.fetch_add(1,
            std::memory_order_relaxed);
}

void RefBase::weakref_type::decWeak(const void *id) {
    LOGE("decWeak");
    weakref_impl* const impl = static_cast<weakref_impl*>(this);

    const int32_t c = impl->mWeak.fetch_sub(1,std::memory_order_release);


    if(c != 1){
        return;
    }
    std::atomic_thread_fence(std::memory_order_acquire);
    int32_t flags = impl->mFlags.load(std::memory_order_release);

    if((flags&OBJECT_LIFETIME_MASK) == OBJECT_LIFETIME_STRONG){
        if(impl->mStrong.load(std::memory_order_release)
        == INITIAL_STRONG_VALUE){
            //说明强引用指针只是初始化
        } else{
            //删除引用计数对象
            LOGE("delete impl");
            delete impl;
        }
    } else{
        impl->mBase->onLastWeakRef(id);
        delete impl->mBase;
    }

}

bool RefBase::weakref_type::attemptIncStrong(const void *id) {
    LOGE("IncStrong");
    incWeak(id);

    weakref_impl*const impl = static_cast<weakref_impl*>(this);

    int32_t curCount = impl->mStrong.load(std::memory_order_relaxed);

    //这种情况是有本已经有数据引用
    while(curCount >0 &&curCount != INITIAL_STRONG_VALUE){
        //发现和原来相比大于1则退出循环
        if(impl->mStrong.compare_exchange_weak(curCount,curCount+1,
                std::memory_order_relaxed)){
            break;
        }
    }

    //这种情况是初始化，或者已经被释放了
    if(curCount<=0 || curCount == INITIAL_STRONG_VALUE){
        int32_t flags = impl->mFlags.
                load(std::memory_order_relaxed);

        if((flags&OBJECT_LIFETIME_MASK) == OBJECT_LIFETIME_STRONG){
            //原来的强引用被释放
            if(curCount <= 0){
                decWeak(id);
                return false;
            }

            //初始化
            while (curCount > 0){
                if(impl->mStrong.compare_exchange_weak(curCount,
                        curCount+1,std::memory_order_relaxed)){
                    break;
                }
            }


            //promote 升级失败
            //避免某些线程，又把当前的sp释放掉
            if(curCount <= 0){
                decWeak(id);
                return false;
            }

        } else{
            //会判断当前是否是需要FIRST_INC_STRONG
            if(!impl->mBase->onIncStrongAttempted(FIRST_INC_STRONG,id)){
                decWeak(id);
                return false;
            }

            curCount = impl->mStrong.load(std::memory_order_relaxed);

            //如果已经初始化过了引用计数，则调用onLastStrongRef
            if(curCount != 0&&curCount!=INITIAL_STRONG_VALUE){
                impl->mBase->onLastStrongRef(id);
            }
        }
    }

    //如果在添加之前是INITIAL_STRONG_VALUE，说明是初始化，
    // 需要减掉INITIAL_STRONG_VALUE，才是真正的计数
    if(curCount == INITIAL_STRONG_VALUE){
        impl->mStrong.fetch_sub(INITIAL_STRONG_VALUE,std::memory_order_relaxed);
    }

    return true;
}

bool RefBase::weakref_type::attemptIncWeak(const void *id) {
    weakref_impl* const impl = static_cast<weakref_impl*>(this);
    int32_t curCount = impl->mWeak.load(std::memory_order_relaxed);
    while(curCount > 0){
        if(impl->mWeak.compare_exchange_weak(curCount,
                curCount+1,std::memory_order_relaxed)){
            break;
        }
    }


    return curCount > 0;
}

int32_t RefBase::weakref_type::getWeakCount() const {
    return static_cast<const weakref_impl*>(this)->mWeak.load(std::memory_order_relaxed);
}
