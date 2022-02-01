#define NODE_ADDON_API_DISABLE_DEPRECATED 1
#include <napi.h>
#include <malloc.h>
#include <features.h>
#include <climits>
#include <string.h>

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)

using namespace Napi;

// standard malloc stuff
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
  inline size_t fixWrap(int val) { return ((val < 0) ? ((size_t)INT_MAX - val) : val); }
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

Value mallocGetHeapUsage(const CallbackInfo& info)
{
    Env env = info.Env();
    Object obj = Object::New(env);
#if __GLIBC_MINOR__ < 33
    auto stats = mallinfo();
#else
    auto stats = mallinfo2();
#endif
    obj.Set("used", fixWrap(stats.uordblks)); /* Total allocated space (bytes) */
    obj.Set("total", fixWrap(stats.arena));   /* Total free + total allocated */
    return obj;
}

Object mallocCreateNamespace(Env env) {
    auto ns = Object::New(env);
    ns.Set("info", Function::New(env, mallocInfo));
    ns.Set("stats", Function::New(env, mallocStats));
    ns.Set("mallinfo2", Function::New(env, mallInfo2));
    ns.Set("trim", Function::New(env, mallocTrim));
    ns.Set("heapUsage", Function::New(env, mallocGetHeapUsage));
    return ns;
}
// ==== end standard malloc ====

// ==== jemalloc stuff ====
extern "C" int __attribute__((weak)) mallctl(const char *name, void *oldp, size_t *oldlenp, void *newp, size_t newlen);

//strerrorname_np is defined in glibc 32
#if (__GLIBC_MINOR__ < 32)
    #define strerrorname_np strerror
#endif

#define THROW_ON_ERROR(err)                                                 \
    if (err) {                                                              \
        Error::New(env, strerrorname_np(err)).ThrowAsJavaScriptException(); \
        return env.Undefined();                                             \
    }

template <typename T>
Value jeDoRead(const char* name, Napi::Env& env)
{
    T result;
    size_t len = sizeof(result);
    auto err = mallctl(name, &result, &len, nullptr, 0);
    THROW_ON_ERROR(err);
    return Value::From(env, result);
}
template<>
Value jeDoRead<void>(const char* name, Napi::Env& env)
{
    auto err = mallctl(name, nullptr, 0, nullptr, 0);
    THROW_ON_ERROR(err);
    return env.Undefined();
}

template <typename T>
Value jeRead(const CallbackInfo& info)
 {
    Napi::Env env = info.Env();
    if (info.Length() != 1) {
        Error::New(env, "Wrong number of arguments, must be 1").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Value arg = info[0];
    if (!arg.IsString()) {
        Error::New(env, "Argument is not a string").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    return jeDoRead<T>(arg.ToString().Utf8Value().c_str(), env);
}

template<typename T, typename JT>
Value jeDoWrite(const char* name, Value jsVal, Napi::Env& env)
{
    T val = static_cast<JT>(jsVal.As<Number>());
    auto err = mallctl(name, nullptr, 0, &val, sizeof(val));
    THROW_ON_ERROR(err);
    return env.Undefined();
}

template <typename T, typename JT>
Value jeWrite(const CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() != 2) {
        Error::New(env, "Wrong number of arguments, must be 2").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Value name = info[0];
    if (!name.IsString()) {
        Error::New(env, "Argument 1 is not a string").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    return jeDoWrite<T, JT>(name.ToString().Utf8Value().c_str(), info[1], env);
}

void jenFlushThreadCache()
{
    mallctl("thread.tcache.flush", NULL, NULL, NULL, 0);
}

int jenUpdateEpoch()
{
    uint64_t epoch = time(nullptr);
    return mallctl("epoch", nullptr, 0, &epoch, sizeof(epoch));
}

Value jeUpdateEpoch(const CallbackInfo& info)
{
    Env env = info.Env();
    auto err = jenUpdateEpoch();
    THROW_ON_ERROR(err);
    return env.Undefined();
}

Value jeGetHeapUsage(const CallbackInfo& info)
{
    Env env = info.Env();
    jenUpdateEpoch();
    size_t used, total, retained;
/*
    see https://github.com/jemalloc/jemalloc/issues/1882#issuecomment-662745494
    - allocated is bytes used by application
    - active is similar but includes the whole pages, and is multiple of page size
    - "Mapped is the sum of regions of virtual address space currently dedicated (internally)
    to serving some live allocation. Some of those regions have pages we are reasonably confident
    have not been demand-paged in yet; these count towards mapped, but not resident.
    Some pages have been touched by user code and then freed, but not yet returned to the OS
    (we're keeping them around under the hope that we'll be able to serve another allocation out of
    them soon). These pages don't hold any allocations on them, so they don't count towards mapped;
    they do however count towards resident."
    - retained: Total number of bytes in virtual memory mappings that were retained rather
    than being returned to the operating system via e.g. munmap(2) or similar. Retained virtual
    memory is typically untouched, decommitted, or purged, so it has no strongly associated
    physical memory (see extent hooks for details). Retained memory is excluded from mapped
    memory statistics
    - mapped > active > allocated
*/
    size_t len = sizeof(used);
    auto err = mallctl("stats.allocated", &used, &len, nullptr, 0);
    THROW_ON_ERROR(err);

    len = sizeof(total);
    err = mallctl("stats.mapped", &total, &len, nullptr, 0);
    THROW_ON_ERROR(err);

    len = sizeof(retained);
    err = mallctl("stats.retained", &retained, &len, nullptr, 0);
    THROW_ON_ERROR(err);
    total += retained;

    Object obj = Object::New(env);
    obj.Set("used", used);
    obj.Set("total", total);
    return obj;
}

Value jeFlushThreadCache(const CallbackInfo& info)
{
    jenFlushThreadCache();
    return info.Env().Undefined();
}
Object jeCreateNamespace(Env env) {
    auto ns = Object::New(env);
    ns.Set("ctlGetSize", Function::New(env, jeRead<size_t>));
    ns.Set("ctlGetSSize", Function::New(env, jeRead<ssize_t>));
    ns.Set("ctlGetU32", Function::New(env, jeRead<uint32_t>));
    ns.Set("ctlGetU64", Function::New(env, jeRead<uint64_t>));
    ns.Set("ctlGetString", Function::New(env, jeRead<const char*>));
    ns.Set("ctlGetBool", Function::New(env, jeRead<bool>));
    ns.Set("ctlGetUnsigned", Function::New(env, jeRead<unsigned>));
    ns.Set("ctlSetSize", Function::New(env, jeWrite<size_t, int64_t>));
    ns.Set("ctlSetSSize", Function::New(env, jeWrite<ssize_t, int64_t>));
    ns.Set("ctlSetUnsigned", Function::New(env, jeWrite<unsigned, int64_t>));
    ns.Set("flushThreadCache", Function::New(env, jeFlushThreadCache));
    ns.Set("updateStats", Function::New(env, jeUpdateEpoch));
    return ns;
}
Object Init(Env env, Object exports)
{
    exports.Set("glibcVersion", String::New(env, STRINGIFY(__GLIBC__) "." STRINGIFY(__GLIBC_MINOR__)));
    exports.Set("malloc", mallocCreateNamespace(env));
    if (mallctl) {
        exports.Set("jemalloc", jeCreateNamespace(env));
        exports.Set("getHeapUsage", Function::New(env, jeGetHeapUsage));
        exports.Set("allocator", String::New(env, "jemalloc"));
    }
    else {
        exports.Set("getHeapUsage", Function::New(env, mallocGetHeapUsage));
        exports.Set("allocator", String::New(env, "malloc"));
    }
    return exports;
}


NODE_API_MODULE(malloc_tools_native, Init);
