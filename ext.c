#include <quickjs/quickjs.h>
#include <poll.h>

#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))

static JSValue js_poll(JSContext *ctx, JSValueConst this_val,
                       int argc, JSValueConst *argv) 
{
    JSValue rs;
    int ret;
    int except = 0;
    struct pollfd pollfds[64];
    if (argc != 2)
        return JS_EXCEPTION;
    if (!JS_IsArray(ctx, argv[0]))
        return JS_EXCEPTION;
    
    if (!JS_IsArray(ctx, argv[0]))
        return JS_EXCEPTION;
    rs = JS_GetPropertyStr(ctx, argv[0], "length");
    if (JS_IsException(rs))
        return rs;
    int len;
    ret = JS_ToUint32(ctx, (uint32_t*)&len, rs);
    JS_FreeValue(ctx, rs);
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
        JSValue param = JS_GetPropertyUint32(ctx, argv[0], i);
        JSValue fd_val = JS_EXCEPTION;
        JSValue events_val = JS_EXCEPTION;
        if (JS_IsException(param)) {
            goto EXCEPTION;
        }
        fd_val = JS_GetPropertyStr(ctx, param, "fd");
        if (JS_IsException(fd_val)) {
            goto EXCEPTION;
        }
        events_val = JS_GetPropertyStr(ctx, param, "events");
        if (JS_IsException(events_val)) {
            goto EXCEPTION;
        }
        
        
        if (JS_IsNumber(fd_val)) {
            if (JS_ToInt32(ctx, (int32_t*)&pollfds[i].fd, fd_val)) {
                goto EXCEPTION;
            }
        }

        if (JS_IsNumber(events_val)) {
            if (JS_ToInt32(ctx, (int32_t*)&pollfds[i].events, events_val)) {
                goto EXCEPTION;
            }
        }
        
        max_len++;
        
    EXCEPTION:
        
        if (!JS_IsException(param)) {
            JS_FreeValue(ctx, param);
        } else {
            except = 1;
        }
        if (!JS_IsException(fd_val)) {
            JS_FreeValue(ctx, fd_val);
        } else {
            except = 1;
        }
        if (!JS_IsException(events_val)) {
            JS_FreeValue(ctx, events_val);
        } else {
            except = 1;
        }
        if (except == 1)
            return JS_EXCEPTION;
    }

    ret = poll(pollfds, max_len, timeout);
    if (ret > 0) {
        for (int i = 0; i < ret; i++) {
            JSValue param = JS_GetPropertyUint32(ctx, argv[0], i);
            JS_FreeValue(ctx, param);
            if (JS_IsException(param)) {
                return param;
            }
            JSValue revents = JS_NewInt32(ctx, pollfds[i].revents);
            JS_SetPropertyStr(ctx, param, "revents", revents);
            JS_FreeValue(ctx, revents);
        }
    }
    rs = JS_NewInt32(ctx, ret);
    return rs;
}

static const JSCFunctionListEntry js_ext_funcs[] = {
    JS_CFUNC_DEF("poll", 1, js_poll),
};

static int js_ext_init(JSContext *ctx, JSModuleDef *module_def) 
{
    return JS_SetModuleExportList(ctx, module_def, js_ext_funcs, ARRAYSIZE(js_ext_funcs));
}

#ifdef JS_SHARED_LIBRARY
#define JS_INIT_MODULE js_init_module
#else
#define JS_INIT_MODULE js_init_module_ext
#endif

JSModuleDef *JS_INIT_MODULE(JSContext *ctx, const char *module_name) 
{
    JSModuleDef *module_def;
    module_def = JS_NewCModule(ctx, module_name, js_ext_init);
    if (!module_def)
        return NULL;
    JS_AddModuleExportList(ctx, module_def, js_ext_funcs, ARRAYSIZE(js_ext_funcs));
    return module_def;
}

