#define NODE_ADDON_API_DISABLE_DEPRECATED 1
#include <napi.h>
#include <malloc.h>
#include <features.h>
#include <climits>

using namespace Napi;

class AutoMemFile {
protected:
    char* mBuf = nullptr;
    size_t mSize = 0;
    FILE* mFile;
public:
    const char* buf() const { return mBuf; }
    size_t size() const { return mSize; }
    FILE* file() const { return mFile; }
    AutoMemFile() {
        mFile = open_memstream(&mBuf, &mSize);
    }
    ~AutoMemFile() {
        if (mFile) {
            fclose(mFile);
        }
        if (mBuf) {
            free(mBuf);
        }
    }
};

Value mallocInfo(const CallbackInfo& info) {
    Env env = info.Env();
    AutoMemFile file;
    if (!file.file()) {
        Error::New(env, "Error creating memory file").ThrowAsJavaScriptException();
        return env.Null();
    }
    auto ret = malloc_info(0, file.file());
    if (ret) {
        Error::New(env, "malloc_info native call returned error").ThrowAsJavaScriptException();
        return env.Null();
    }
    fflush(file.file());
    return String::New(env, file.buf(), file.size());
}

void mallocStats(const CallbackInfo& info) {
    malloc_stats();
}

// Fix integer wrapping when value is > 2G
#if __GLIBC_MINOR__ < 33
  inline size_t fixWrap(int val) { return ((val < 0) ? (INT_MAX - val) : val); }
#else
  inline size_t fixWrap(size_t val) { return val; }
#endif

Value mallInfo2(const CallbackInfo& info) {
    Env env = info.Env();
    Object obj = Object::New(env);
#if __GLIBC_MINOR__ < 33
#warning Glibc version is older than 2.33, will use mallinfo instead of mallinfo2
    auto stats = mallinfo();
#else
    auto stats = mallinfo2();
#endif
    obj.Set("arena", fixWrap(stats.arena));      /* Non-mmapped space allocated (bytes) */
    obj.Set("ordblks", fixWrap(stats.ordblks));  /* Number of free chunks */
    obj.Set("smblks", fixWrap(stats.smblks));    /* Number of free fastbin blocks */
    obj.Set("hblks", fixWrap(stats.hblks));      /* Number of mmapped regions */
    obj.Set("hblkhd", fixWrap(stats.hblkhd));     /* Space allocated in mmapped regions (bytes) */
    obj.Set("usmblks", fixWrap(stats.usmblks));   /* See below */
    obj.Set("fsmblks", fixWrap(stats.fsmblks));   /* Space in freed fastbin blocks (bytes) */
    obj.Set("uordblks", fixWrap(stats.uordblks)); /* Total allocated space (bytes) */
    obj.Set("fordblks", fixWrap(stats.fordblks)); /* Total free space (bytes) */
    obj.Set("keepcost", fixWrap(stats.keepcost)); /* Top-most, releasable space (bytes) */
    return obj;
}
Value mallocTrim(const CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() != 1) {
        Error::New(env, "Wrong number of arguments, must be 1").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Value arg = info[0];
    if (!arg.IsNumber()) {
        Error::New(env, "Argument is not a number").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto ret = malloc_trim(arg.As<Number>().Int32Value());
    return Number::New(env, ret);
}
Object Init(Env env, Object exports) {
    exports.Set("malloc_info", Function::New(env, mallocInfo));
    exports.Set("malloc_stats", Function::New(env, mallocStats));
    exports.Set("mallinfo2", Function::New(env, mallInfo2));
    exports.Set("malloc_trim", Function::New(env, mallocTrim));
    return exports;
}

NODE_API_MODULE(malloc_tools_native, Init);
