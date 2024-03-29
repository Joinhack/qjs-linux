#include <quickjs/quickjs.h>
#include <poll.h>
#include <fcntl.h>

#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))

static JSValue js_fnonblock(JSContext *ctx, JSValueConst this_val,
                       int argc, JSValueConst *argv) {
    JSValue rs;
    int ret, fd, flags;
    if (argc < 1)
        return JS_EXCEPTION;
    
    if (!JS_IsNumber(argv[0]))
        return JS_EXCEPTION;
    ret = JS_ToInt32(ctx, (int32_t*)&fd, argv[0]);
    if (ret)
        return JS_EXCEPTION;
    flags = fcntl(fd, F_GETFL);
    if (flags < 0)
        return JS_EXCEPTION;
    flags |= O_NONBLOCK;
    ret = fcntl(fd, F_SETFL, flags);
    if (ret < 0)
        return JS_EXCEPTION;
    rs = JS_NewInt32(ctx, ret);
    return rs;
}

class JSValueGuard {
    JSValue &handle;
    JSContext *ctx;
public:
    JSValueGuard(JSContext *ctx, JSValue &_handle): handle(_handle) {
        this->ctx = ctx;
    }
    ~JSValueGuard() {
        if (!IsException())
            JS_FreeValue(ctx, handle);
    }
    inline bool IsException() {
        return JS_IsException(handle);
    }
    inline bool IsNumber() {
        return JS_IsNumber(handle);
    }
    JSValue& operator*() {
        return handle;
    }
};

static JSValue js_poll(JSContext *ctx, 
                        JSValueConst this_val,
                        int argc, 
                        JSValueConst *argv) {
    JSValue rs;
    int ret;
    int except = 0;
    struct pollfd pollfds[64];
    if (argc != 2)
        return JS_EXCEPTION;
    if (!JS_IsArray(ctx, argv[0]))
        return JS_EXCEPTION;
    
    rs = JS_GetPropertyStr(ctx, argv[0], "length");
    JSValueGuard lenJSVal(ctx, rs);
    if (lenJSVal.IsException())
        return *lenJSVal;
    int len;
    ret = JS_ToUint32(ctx, (uint32_t*)&len, *lenJSVal);
    if (ret)
        return JS_EXCEPTION;
    int max_len = 0;
    if (!JS_IsNumber(argv[1]))
        return JS_EXCEPTION;
    int timeout;
    ret = JS_ToInt32(ctx, (int32_t*)&timeout, argv[1]);
    if (ret)
        return JS_EXCEPTION;

    for (int i = 0; i < len; i++) {
        JSValue _param = JS_GetPropertyUint32(ctx, argv[0], i);
        JSValueGuard param(ctx, _param);
        JSValue events_val = JS_EXCEPTION;
        if (param.IsException()) {
            return JS_EXCEPTION;
        }
        JSValue _fdVal = JS_GetPropertyStr(ctx, *param, "fd");
        JSValueGuard fdVal(ctx, _fdVal);
        if (fdVal.IsException()) {
            return JS_EXCEPTION;
        }
        JSValue _eventsVal = JS_GetPropertyStr(ctx, *param, "events");
        JSValueGuard eventsVal(ctx, _eventsVal);
        if (eventsVal.IsException()) {
            return JS_EXCEPTION;
        }
        
        if (fdVal.IsNumber()) {
            if (JS_ToInt32(ctx, (int32_t*)&pollfds[i].fd, *fdVal)) {
                return JS_EXCEPTION;
            }
        }

        if (eventsVal.IsException()) {
            if (JS_ToInt32(ctx, (int32_t*)&pollfds[i].events, events_val)) {
                return JS_EXCEPTION;
            }
        }
        max_len++;
    }

    ret = poll(pollfds, max_len, timeout);
    if (ret > 0) {
        for (int i = 0; i < ret; i++) {
            JSValue _param = JS_GetPropertyUint32(ctx, argv[0], i);
            JSValueGuard param(ctx, _param);
            if (param.IsException()) {
                return JS_EXCEPTION;
            }
            JSValue _revents = JS_NewInt32(ctx, pollfds[i].revents);
            JSValueGuard revents(ctx, _revents);
            JS_SetPropertyStr(ctx, *param, "revents", *revents);
        }
    }
    rs = JS_NewInt32(ctx, ret);
    return rs;
}

static const JSCFunctionListEntry js_ext_funcs[] = {
    JS_CFUNC_DEF("poll", 1, js_poll),
    JS_CFUNC_DEF("fnonblock", 1, js_fnonblock),
};

static int js_ext_init(JSContext *ctx, JSModuleDef *module_def) {
    return JS_SetModuleExportList(ctx, module_def, js_ext_funcs, ARRAYSIZE(js_ext_funcs));
}

extern "C" {
#ifdef JS_SHARED_LIBRARY
#define JS_INIT_MODULE js_init_module
#else
#define JS_INIT_MODULE js_init_module_ext
#endif


JSModuleDef *JS_INIT_MODULE(JSContext *ctx, const char *module_name) {
    JSModuleDef *module_def;
    module_def = JS_NewCModule(ctx, module_name, js_ext_init);
    if (!module_def)
        return NULL;
    JS_AddModuleExportList(ctx, module_def, js_ext_funcs, ARRAYSIZE(js_ext_funcs));
    return module_def;
}
}
