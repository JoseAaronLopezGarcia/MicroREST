//
// Created by aaron on 26/3/24.
//
#include <jni.h>

#include "server.h"

static JavaVM* jvm;

static std::string jstring2string(JNIEnv* env, jstring jstr){
    //  javap -s -public java.lang.String | egrep -A 2 "getBytes"
    const auto stringClass = env->FindClass("java/lang/String");
    const auto getBytes = env->GetMethodID(stringClass, "getBytes", "()[B");

    const auto stringJbytes = (jbyteArray) env->CallObjectMethod(jstr, getBytes);

    const auto length = env->GetArrayLength(stringJbytes);
    const auto pBytes = env->GetByteArrayElements(stringJbytes, nullptr);
    std::string s((char *)pBytes, length);
    env->ReleaseByteArrayElements(stringJbytes, pBytes, JNI_ABORT);
    return s;
}

static RequestResponse jni_handler(void* ctx, std::string method, std::string body){

    printf("server jni handler\n");

    JNIEnv* pEnv;
    jobject pHandler = (jobject)ctx;

    JavaVMAttachArgs vmargs;
    vmargs.version = JNI_VERSION_1_6; // choose your JNI version
    vmargs.name = NULL; // you might want to give the java thread a name
    vmargs.group = NULL; // you might want to assign the java thread to a ThreadGroup
    jvm->AttachCurrentThread( &pEnv , &vmargs);

    jclass klass = pEnv->GetObjectClass(pHandler);

    // call getClass()
    printf("call handle()\n");
    jstring jmethod = pEnv->NewStringUTF(method.c_str());
    jstring jbody = pEnv->NewStringUTF(body.c_str());
    jmethodID handleMethodId = pEnv->GetMethodID(klass,"handle","(Ljava/lang/String;Ljava/lang/String;)V");
    pEnv->CallVoidMethod(pHandler, handleMethodId, jmethod, jbody);

    // call getCode()
    printf("call getCode()\n");
    jmethodID codeMethodId = pEnv->GetMethodID(klass,"getCode","()I");
    jint jcode = pEnv->CallIntMethod(pHandler, codeMethodId);

    // call getBody()
    printf("call getResponse()\n");
    jmethodID bodyMethodId = pEnv->GetMethodID(klass,"getResponse","()Ljava/lang/String;");
    jstring jres = (jstring)pEnv->CallObjectMethod(pHandler, bodyMethodId);

    // parse java string
    std::string body_res = jstring2string(pEnv, jres);

    jvm->DetachCurrentThread();

    return {
            (int)jcode,
            body_res
    };
}

extern "C" JNIEXPORT void JNICALL
Java_org_MicroREST_NativeServer_setAuthorizationToken(
        JNIEnv* pEnv,
        jclass pThis,
        jstring pToken
) {
    pEnv->GetJavaVM(&jvm);

    std::string t = jstring2string(pEnv, pToken);
    set_authorization_token(t.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_org_MicroREST_NativeServer_enableTest(
        JNIEnv* pEnv,
        jclass pThis
) {
    pEnv->GetJavaVM(&jvm);
    enable_hello_test();
}

extern "C" JNIEXPORT void JNICALL
Java_org_MicroREST_NativeServer_registerHandler(
        JNIEnv* pEnv,
        jclass pThis,
        jstring pPath,
        jobject pHandler
        ) {
    pEnv->GetJavaVM(&jvm);

    std::string path = jstring2string(pEnv, pPath);

    RequestHandler handler = {
            path,
            (void*)pEnv->NewGlobalRef(pHandler),
            &jni_handler
    };

    register_handler(handler);
}

extern "C" JNIEXPORT void JNICALL
Java_org_MicroREST_NativeServer_startServer(
        JNIEnv* pEnv,
        jclass pThis,
        jint pPort) {
    pEnv->GetJavaVM(&jvm);
    start_server_thread((int)pPort);
}
