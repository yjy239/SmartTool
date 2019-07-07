//
// Created by 余均宇 on 2019-06-30.
//

#ifndef SMARTTOOLS_MUTEX_H
#define SMARTTOOLS_MUTEX_H


#include <strings.h>
#include <pthread.h>
#include "Define.h"
class Mutex {
public:
    Mutex() {
        pthread_mutex_init(&mMutex, NULL);
    }
    int lock() {
        return pthread_mutex_lock(&mMutex);
    }
    void unlock() {
        pthread_mutex_unlock(&mMutex);
    }
    ~Mutex() {
        pthread_mutex_destroy(&mMutex);
    }

    // A simple class that locks a given mutex on construction
    // and unlocks it when it goes out of scope.
    class Autolock {
    public:
        Autolock(Mutex &mutex) : lock(&mutex) {
            lock->lock();
            LOGE("lock");
        }
        ~Autolock() {
            lock->unlock();
            LOGE("unlock");
        }
    private:
        Mutex *lock;
    };

private:
    pthread_mutex_t mMutex;

    // Disallow copy and assign.
    Mutex(const Mutex&);
    Mutex& operator=(const Mutex&);
};


#endif //SMARTTOOLS_MUTEX_H
