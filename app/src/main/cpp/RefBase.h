//
// Created by 余均宇 on 2019-07-05.
//

#ifndef SMARTTOOLS_REFBASE_H
#define SMARTTOOLS_REFBASE_H

#include <stdio.h>
#include <StrongPointer.h>

#define COMPARE_WEAK(_op_)                                      \
inline bool operator _op_ (const sp<T>& o) const {              \
    return m_ptr _op_ o.m_ptr;                                  \
}                                                               \
inline bool operator _op_ (const T* o) const {                  \
    return m_ptr _op_ o;                                        \
}                                                               \
template<typename U>                                            \
inline bool operator _op_ (const sp<U>& o) const {              \
    return m_ptr _op_ o.m_ptr;                                  \
}                                                               \
template<typename U>                                            \
inline bool operator _op_ (const U* o) const {                  \
    return m_ptr _op_ o;                                        \
}

class RefBase {
public:
    void incStrong(const void* id) const;

    void decStrong(const void* id) const;

    int32_t getStrongCount(const void* id) const;

    void forceIncStrong(const void* id) const;


    class weakref_type{
    public:
        RefBase* refBase() const;

        void incWeak(const void* id);

        void decWeak(const void* id);

        // acquires a strong reference if there is already one.
        bool attemptIncStrong(const void* id);

        bool attemptIncWeak(const void* id);

        int32_t getWeakCount() const;

    };

    weakref_type* createWeak(const void* id) const;

    weakref_type* getWeakRef()const;

protected:
    RefBase();

    virtual ~RefBase();

    enum {
        OBJECT_LIFETIME_STRONG = 0x0000,
        OBJECT_LIFETIME_WEAK = 0x0001,
        OBJECT_LIFETIME_MASK = 0x0001
    };

    void extendObjectLifetime(int32_t mode);

    enum {
        FIRST_INC_STRONG = 0x0001
    };

    virtual void onFirstRef();

    virtual void onLastStrongRef(const void* id);

    virtual bool onIncStrongAttempted(int32_t flag, const void* id);

    virtual void onLastWeakRef(const void* id);

private:
    //为了让weakref_type去访问到refbase中的私有数据
    friend class weakref_type;
    //一个实现类
    class weakref_impl;
    RefBase(const RefBase& o);

    RefBase& operator =(const RefBase& o);

    weakref_impl *const mRefs;
};


//---------------------

template <typename T>
class wp{
public:
    typedef typename RefBase::weakref_type weakref_type;

    inline wp():m_ptr(0){}

    wp(T* other);
    //拷贝构造函数
    wp(const wp<T>& other);

    explicit wp(const sp<T>& other);


    template <typename U> wp(U* other);

    template <typename U> wp(const sp<U>& other);

    template <typename U> wp(const wp<U>& other);


    ~wp();


    wp<T>& operator = (T* other);
    wp<T>& operator = (const wp<T>& other);
    wp<T>& operator = (const sp<T>& other);

    template<typename U> wp& operator = (U* other);
    template<typename U> wp& operator = (const wp<U>& other);
    template<typename U> wp& operator = (const sp<U>& other);

    void set_object_and_refs(T* other, weakref_type* refs);

    // promotion to sp

    sp<T> promote() const;


    // Reset

    void clear();

    // Accessors

    inline  weakref_type* get_refs() const { return m_refs; }

    inline  T* unsafe_get() const { return m_ptr; }

    // Operators
//
    COMPARE_WEAK(==)
    COMPARE_WEAK(!=)
    COMPARE_WEAK(>)
    COMPARE_WEAK(<)
    COMPARE_WEAK(<=)
    COMPARE_WEAK(>=)

    inline bool operator == (const wp<T>& o) const {
        return (m_ptr == o.m_ptr) && (m_refs == o.m_refs);
    }
    template<typename U>
    inline bool operator == (const wp<U>& o) const {
        return m_ptr == o.m_ptr;
    }

    inline bool operator > (const wp<T>& o) const {
        return (m_ptr == o.m_ptr) ? (m_refs > o.m_refs) : (m_ptr > o.m_ptr);
    }
    template<typename U>
    inline bool operator > (const wp<U>& o) const {
        return (m_ptr == o.m_ptr) ? (m_refs > o.m_refs) : (m_ptr > o.m_ptr);
    }

    inline bool operator < (const wp<T>& o) const {
        return (m_ptr == o.m_ptr) ? (m_refs < o.m_refs) : (m_ptr < o.m_ptr);
    }
    template<typename U>
    inline bool operator < (const wp<U>& o) const {
        return (m_ptr == o.m_ptr) ? (m_refs < o.m_refs) : (m_ptr < o.m_ptr);
    }
    inline bool operator != (const wp<T>& o) const { return m_refs != o.m_refs; }
    template<typename U> inline bool operator != (const wp<U>& o) const { return !operator == (o); }
    inline bool operator <= (const wp<T>& o) const { return !operator > (o); }
    template<typename U> inline bool operator <= (const wp<U>& o) const { return !operator > (o); }
    inline bool operator >= (const wp<T>& o) const { return !operator < (o); }
    template<typename U> inline bool operator >= (const wp<U>& o) const { return !operator < (o); }


private:
    template <typename Y> friend class wp;
    template <typename Y> friend class sp;
    T* m_ptr;
    weakref_type* m_refs;
};

#undef COMPARE_WEAK

template <typename T>
wp<T>::wp(T *other):m_ptr(other){
    if(other){
        m_refs = other->createWeak(this);
    }
}


template <typename T>
wp<T>::wp(const wp<T>& other)
:m_ptr(other.m_ptr),m_refs(other.m_refs){
    //other的指针不为空，再增加弱引用计数
    if(m_ptr){
        m_refs->incWeak(this);
    }
}

template <typename T>
wp<T>::wp(const sp<T>& other):m_ptr(other.m_ptr){
    if(m_ptr){
        m_refs = m_ptr->createWeak(this);
    }
}

template <typename T> template <typename U>
wp<T>::wp(U *other)
:m_ptr(other){
    if(other){
        m_refs = other->createWeak(this);
    }
}


template <typename T> template <typename U>
wp<T>::wp(const wp<U>& other)
:m_ptr(other.m_ptr){
    if(m_ptr){
        m_refs = other.m_refs;
        m_refs->incWeak(this);
    }
}

template <typename T> template <typename U>
wp<T>::wp(const sp<U>& other)
:m_ptr(other.m_ptr){
    if(m_ptr){
        m_refs = m_ptr->createWeak(this);
    }
}


template <typename T>
wp<T>::~wp() {
    if(m_ptr){
        m_refs->decWeak(this);
    }
}


template <typename T>
wp<T>& wp<T>::operator=(T *other) {
    //赋值操作，把带着RefBase的对象复制给弱引用
    //为新的对象创建引用计数器
    weakref_type* newRefs = other ? other->createWeak(this) : 0;
    //如果原来的指针有数据，则需要把原来的弱引用减一。
    //因为此时相当于把当前已有的弱引用被新来的替换掉
    //那么，原来引用的弱引用计数要减一
    if(m_ptr){
        m_refs->decWeak(this);
    }


    m_ptr = other;
    m_refs = newRefs;
    return *this;
}


template <typename T>
wp<T>& wp<T>::operator=(const wp<T> &other) {
    //弱引用赋值
    weakref_type* otherRef(other.m_refs);
    T* otherPtr(other.m_ptr);
    if(otherPtr){
        otherPtr->incWeak(this);
    }

    if(m_ptr){
        m_refs->decWeak(this);
    }

    m_ptr = otherPtr;
    m_refs = otherRef;
    return *this;
}

template <typename T>
wp<T>& wp<T>::operator=(const sp<T> &other) {
    //强引用赋值给弱引用
    //和上面对象赋值同理
    weakref_type* newRefs = other ? other->createWeak(this) : 0;
    T* otherPtr(other.m_ptr);
    if(m_ptr){
        m_refs->decWeak(this);
    }

    m_ptr = otherPtr;
    m_refs = newRefs;
    return *this;
}

template <typename T> template <typename U>
wp<T>& wp<T>::operator=(U *other) {
    //不是同类型赋值给弱引用
    weakref_type* newRefs = other ? other->createWeak(this) : 0;
    if(m_ptr){
        m_refs->decWeak(this);
    }

    m_ptr = other;
    m_refs = newRefs;
    return *this;
}

template<typename T> template<typename U>
wp<T>& wp<T>::operator = (const wp<U>& other)
{
    //不同类型的弱引用赋值
    weakref_type* otherRefs(other.m_refs);
    U* otherPtr(other.m_ptr);
    if (otherPtr){
        otherRefs->incWeak(this);
    }
    if (m_ptr){
        m_refs->decWeak(this);
    }
    m_ptr = otherPtr;
    m_refs = otherRefs;
    return *this;
}

template<typename T> template<typename U>
wp<T>& wp<T>::operator = (const sp<U>& other)
{
    //不同对象的强引用赋值给弱引用
    weakref_type* newRefs =
            other != NULL ? other->createWeak(this) : 0;
    U* otherPtr(other.m_ptr);
    if (m_ptr){
        m_refs->decWeak(this);
    }
    m_ptr = otherPtr;
    m_refs = newRefs;
    return *this;
}

template<typename T>
void wp<T>::set_object_and_refs(T* other, weakref_type* refs)
{
    //直接赋值对象和引用
    if (other){
        refs->incWeak(this);
    }
    if (m_ptr){
        m_refs->decWeak(this);
    }
    m_ptr = other;
    m_refs = refs;
}

template <typename T>
sp<T> wp<T>::promote() const {
    //核心
    sp<T> result;
    if(m_ptr && m_refs->attemptIncStrong(&result)){
        result.set_pointer(m_ptr);
    }

    return result;
}

template<typename T>
void wp<T>::clear()
{
    if (m_ptr) {
        m_refs->decWeak(this);
        m_ptr = 0;
    }
}

#endif //SMARTTOOLS_REFBASE_H
