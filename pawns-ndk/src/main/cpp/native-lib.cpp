/* =========================================================================
 *  native.cpp  – original file + minimal mock block
 * ========================================================================= */

#include <jni.h>
#include <string>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <android/log.h>
#include <thread>      // added
#include <chrono>      // added
#include <random>      // added
#include <cstring>     // added

#ifdef ARCH_ARM64_V8A
#include "arm64-v8a/libpawns_mobile_sdk.h"
#elif defined(ARCH_ARMEABI_V7A)
#include "armeabi-v7a/libpawns_mobile_sdk.h"
#elif defined(ARCH_X86)
#include "x86/libpawns_mobile_sdk.h"
#elif defined(ARCH_X86_64)
#include "x86_64/libpawns_mobile_sdk.h"
#else
  #error "Unsupported architecture"
#endif

/* ---------------------- original globals / JNI code --------------------- */
static JavaVM *javaVM = nullptr;
static jobject globalCallback = nullptr;
static std::atomic<bool> isCallbackValid{false};
static int activeCallbackCount = 0;
static std::mutex callbackMutex;
static std::condition_variable callbackCondition;

extern "C" {
#define LOG_TAG "NativeLib"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO , LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

void myCallback(char *message) {
    JNIEnv *env = nullptr;
    bool attached = false;
    jobject localCallback = nullptr;

    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        if (!isCallbackValid) {
            LOGI("Callback is no longer valid. Skipping callback invocation.");
            return;
        }
        activeCallbackCount++;
        localCallback = globalCallback;
    }

    if (javaVM->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        if (javaVM->AttachCurrentThread(&env, nullptr) == JNI_OK) {
            attached = true;
        } else {
            LOGE("Failed to attach current thread to Java VM");
            std::lock_guard<std::mutex> lock(callbackMutex);
            activeCallbackCount--;
            callbackCondition.notify_all();
            return;
        }
    }

    do {
        jclass callbackClass = env->GetObjectClass(localCallback);
        if (!callbackClass) { LOGE("Failed to find class of globalCallback"); break; }

        jmethodID onCallbackMethod = env->GetMethodID(callbackClass, "onCallback", "(Ljava/lang/String;)V");
        env->DeleteLocalRef(callbackClass);
        if (!onCallbackMethod) { LOGE("Failed to find method 'onCallback'"); break; }

        jstring jMessage = env->NewStringUTF(message);
        if (!jMessage) { LOGE("Failed to create jstring from message"); break; }

        env->CallVoidMethod(localCallback, onCallbackMethod, jMessage);
        env->DeleteLocalRef(jMessage);

        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
            LOGE("Exception occurred while calling Java callback.");
        }
    } while (false);

    if (attached) javaVM->DetachCurrentThread();

    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        activeCallbackCount--;
        callbackCondition.notify_all();
    }
}

JNIEXPORT void JNICALL
Java_com_pawns_ndk_PawnsCore_Initialize(JNIEnv *env, jobject, jstring rawDeviceID, jstring rawDeviceName) {
    const char *cRawDeviceID   = env->GetStringUTFChars(rawDeviceID, 0);
    const char *cRawDeviceName = env->GetStringUTFChars(rawDeviceName, 0);

    Initialize((char *)cRawDeviceID, (char *)cRawDeviceName);

    env->ReleaseStringUTFChars(rawDeviceID,  cRawDeviceID);
    env->ReleaseStringUTFChars(rawDeviceName,cRawDeviceName);
}

JNIEXPORT void JNICALL
Java_com_pawns_ndk_PawnsCore_StartMainRoutine(JNIEnv *env, jobject, jstring rawAccessToken, jobject callback) {
    const char *nativeAccessToken = env->GetStringUTFChars(rawAccessToken, 0);

    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        if (globalCallback) env->DeleteGlobalRef(globalCallback);
        globalCallback = env->NewGlobalRef(callback);

        if (!javaVM) env->GetJavaVM(&javaVM);
        isCallbackValid.store(true);
    }

    StartMainRoutine((char *)nativeAccessToken, (void *)myCallback);

    env->ReleaseStringUTFChars(rawAccessToken, nativeAccessToken);
}

JNIEXPORT void JNICALL
Java_com_pawns_ndk_PawnsCore_StopMainRoutine(JNIEnv *env, jobject) {
    StopMainRoutine();

    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        isCallbackValid.store(false);
    }

    {
        std::unique_lock<std::mutex> lock(callbackMutex);
        callbackCondition.wait(lock, []{ return activeCallbackCount == 0; });

        if (globalCallback) {
            env->DeleteGlobalRef(globalCallback);
            globalCallback = nullptr;
        }
    }
}

} // extern "C"

/* -------------------------------------------------------------------------
 *                  MOCK IMPLEMENTATION  (appended section)
 * ------------------------------------------------------------------------- */

static std::atomic<bool> gMockRunning{false};
static std::thread       gMockThread;
static void*             gUserCallback = nullptr;        // set in StartMainRoutine

// JSON messages provided by the user
static const char* kMessages[] = {
        R"({"happened_at":"2025-05-26T10:04:16Z","name":"starting","parameters":{}})",
        R"({"happened_at":"2025-05-26T10:04:38Z","name":"running","parameters":{}})",
        R"({"happened_at":"2025-05-26T10:05:06Z","name":"not_running","parameters":{"will_reconnect":"0"}})",
        R"({"happened_at":"2025-05-26T10:04:38Z","name":"running","parameters":{}})",
        R"({"happened_at":"2025-05-26T10:07:27Z","name":"not_running","parameters":{"error":"ip_used","message":"start exit node","will_reconnect":"0"}})",
        R"({"happened_at":"2025-05-26T10:04:38Z","name":"running","parameters":{}})",
        R"({"happened_at":"2025-05-26T10:08:37Z","name":"not_running","parameters":{"error":"cant_get_free_port","message":"","will_reconnect":"1"}})",
        R"({"happened_at":"2025-05-26T10:04:38Z","name":"running","parameters":{}})",
        R"({"happened_at":"2025-05-26T10:08:37Z","name":"not_running","parameters":{"error":"could_not_mark_peer_alive","message":"","will_reconnect":"0"}})",
        R"({"happened_at":"2025-05-26T10:04:38Z","name":"running","parameters":{}})",
        R"({"happened_at":"2025-05-26T10:08:37Z","name":"not_running","parameters":{"error":"non_residential_ip","message":"","will_reconnect":"1"}})",
        R"({"happened_at":"2025-05-26T10:04:38Z","name":"running","parameters":{}})",
};
static constexpr size_t kMessageCount = sizeof(kMessages) / sizeof(kMessages[0]);

extern "C" {

// --- Initialize ------------------------------------------------------------
void Initialize(char* rawDeviceID, char* rawDeviceName)
{
    __android_log_print(ANDROID_LOG_INFO, "SDKMOCK",
                        "Mock Initialize (id=%s, name=%s) – real Go init skipped",
                        rawDeviceID ? rawDeviceID : "null",
                        rawDeviceName ? rawDeviceName : "null");
}

// --- StartMainRoutine ------------------------------------------------------
void StartMainRoutine(char* /*accessToken*/, void* callback)
{
    gUserCallback = callback;

    if (gMockRunning.exchange(true)) {   // already running
        __android_log_print(ANDROID_LOG_INFO, "SDKMOCK",
                            "StartMainRoutine called again – generator already active");
        return;
    }

    gMockThread = std::thread([]{
        std::mt19937 rng{std::random_device{}()};
        std::uniform_int_distribution<int> distDelay(5, 120);           // seconds
        std::uniform_int_distribution<int> distMsg(0, kMessageCount-1);
        static char msgBuf[256];

        __android_log_print(ANDROID_LOG_INFO, "SDKMOCK",
                            "Background generator thread started");

        while (gMockRunning.load(std::memory_order_acquire)) {
            int delay = distDelay(rng);
            for (int slept = 0; slept < delay && gMockRunning.load(); ++slept) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (!gMockRunning.load()) break;

            const char* chosen = kMessages[distMsg(rng)];
            std::strncpy(msgBuf, chosen, sizeof(msgBuf)-1);
            msgBuf[sizeof(msgBuf)-1] = '\0';

            if (gUserCallback) {
                reinterpret_cast<void(*)(char*)>(gUserCallback)(msgBuf);
            }
        }

        __android_log_print(ANDROID_LOG_INFO, "SDKMOCK",
                            "Background generator thread exits");
    });
}

// --- StopMainRoutine -------------------------------------------------------
void StopMainRoutine()
{
    if (!gMockRunning.exchange(false)) {   // not active
        __android_log_print(ANDROID_LOG_INFO, "SDKMOCK",
                            "StopMainRoutine called – generator not active");
        return;
    }

    __android_log_print(ANDROID_LOG_INFO, "SDKMOCK",
                        "StopMainRoutine – stopping generator");

    if (gMockThread.joinable()) gMockThread.join();
    gUserCallback = nullptr;
}

} // extern "C"
