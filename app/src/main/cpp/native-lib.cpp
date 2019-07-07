#include <jni.h>
#include <string>
#include <Mutex.h>

#include <Define.h>
#include <LightRefBase.h>
#include <SmartPointer.h>
#include <RefBase.h>
class TestLight :public LightRefBase<TestLight>{
public:
    TestLight(){

    }
};

class Test:public RefBase{
private:
    void onFirstRef(){
        LOGE("first");
    }
public:
    void print(){
        LOGE("PRINT");
    }

    void incStrongPointer(){
        incStrong(this);
    }

    int printSCount(){
        return getStrongCount(this);
    }
};

void testPointer(){
    sp<Test> s1;
    {
        sp<Test> s;
        s = new Test();
        s->print();
        LOGE("1 times:%d",s->printSCount());
        //s->incStrongPointer();
        s1 = s;
        if(s){
            LOGE("2 times:%d",s1->printSCount());
        }


    }


    if(s1){
        LOGE("3 times:%d",s1->printSCount());
    }

}

extern "C" JNIEXPORT jstring JNICALL
Java_com_yjy_smarttools_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";

    Mutex mStateLock;
    {
        Mutex::Autolock l(mStateLock);
    }

    LOGE("do something");

    //SmartPointer<TestLight> sp(new TestLight());
    testPointer();

    return env->NewStringUTF(hello.c_str());
}


