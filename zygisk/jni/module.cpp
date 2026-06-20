#include <fcntl.h>
#include <jni.h>
#include <string.h>
#include <sys/system_properties.h>
#include <unistd.h>

#include "utils.hpp"
#include "zygisk.hpp"

#define LIKELY(value) __builtin_expect(!!(value), 1)
#define UNLIKELY(value) __builtin_expect(!!(value), 0)
#define HOT __attribute__((hot))
#define COLD __attribute__((cold, noinline))

namespace {

using SysPropGetFn = int (*)(const char*, char*);
SysPropGetFn g_orig_system_property_get = nullptr;

HOT int my_system_property_get(const char* name, char* value) {
    if (LIKELY(name != nullptr)) {
        const char first_char = name[0];
        if (UNLIKELY(first_char == 's' || first_char == 'p')) {
            if (strcmp(name, "sys.usb.config") == 0 || strcmp(name, "persist.sys.usb.config") == 0) {
                if (LIKELY(value != nullptr)) {
                    value[0] = 'm'; value[1] = 't'; value[2] = 'p'; value[3] = '\0';
                }
                return 3;
            }
        }
    }
    return g_orig_system_property_get(name, value);
}

using PropCallbackFn = void (*)(void* cookie, const char* name, const char* value, uint32_t serial);
using SysPropReadCallbackFn = int (*)(const void* pi, PropCallbackFn callback, void* cookie);

SysPropReadCallbackFn g_orig_system_property_read_callback = nullptr;

struct CallbackWrapper {
    PropCallbackFn original_callback;
    void* original_cookie;
};

HOT void my_prop_callback(void* cookie, const char* name, const char* value, uint32_t serial) {
    auto* wrapper = static_cast<CallbackWrapper*>(cookie);
    
    if (LIKELY(name != nullptr)) {
        const char first_char = name[0];
        if (UNLIKELY(first_char == 's' || first_char == 'p')) {
            if (strcmp(name, "sys.usb.config") == 0 || strcmp(name, "persist.sys.usb.config") == 0) {
                wrapper->original_callback(wrapper->original_cookie, name, "mtp", serial);
                return;
            }
        }
    }
    wrapper->original_callback(wrapper->original_cookie, name, value, serial);
}

// Interceptor
HOT void my_system_property_read_callback(const void* pi, PropCallbackFn callback, void* cookie) {
    if (LIKELY(callback != nullptr)) {
        CallbackWrapper wrapper = {callback, cookie};
        g_orig_system_property_read_callback(pi, my_prop_callback, &wrapper);
    } else {
        g_orig_system_property_read_callback(pi, callback, cookie);
    }
}

COLD void hookLibrary(zygisk::Api* api, const char* lib_name) {
    ino_t inode = 0;
    dev_t device = 0;
    if (getMapping(lib_name, &inode, &device)) {
        // Hook Legacy API
        api->pltHookRegister(device, inode, "__system_property_get",
                             reinterpret_cast<void*>(&my_system_property_get),
                             reinterpret_cast<void**>(&g_orig_system_property_get));
                             
        // Hook Modern API
        api->pltHookRegister(device, inode, "__system_property_read_callback",
                             reinterpret_cast<void*>(&my_system_property_read_callback),
                             reinterpret_cast<void**>(&g_orig_system_property_read_callback));
    }
}

COLD bool isTargetProcess(zygisk::Api* api, JNIEnv* env, jstring nice_name) {
    if (UNLIKELY(nice_name == nullptr)) return false;

    const jsize len = env->GetStringLength(nice_name);
    if (UNLIKELY(len <= 0 || len >= 256)) return false;

    char process_name[256];
    env->GetStringUTFRegion(nice_name, 0, len, process_name);
    process_name[len] = '\0';

    bool is_target = false;
    const int dir_fd = api->getModuleDir();
    if (LIKELY(dir_fd >= 0)) {
        const int fd = openat(dir_fd, "target.txt", O_RDONLY);
        if (LIKELY(fd >= 0)) {
            char buf[4096];
            const ssize_t bytes = read(fd, buf, sizeof(buf) - 1);
            if (LIKELY(bytes > 0)) {
                buf[bytes] = '\0';
                char* saveptr = nullptr;
                char* line = strtok_r(buf, "\n\r", &saveptr);
                while (line != nullptr) {
                    if (strcmp(line, process_name) == 0) {
                        is_target = true;
                        break;
                    }
                    line = strtok_r(nullptr, "\n\r", &saveptr);
                }
            }
            close(fd);
        }
    }
    return is_target;
}

class AdbSpooferModule final : public zygisk::ModuleBase {
  public:
    void onLoad(zygisk::Api* api, JNIEnv* env) override {
        api_ = api;
        env_ = env;
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs* args) override {
        if (UNLIKELY(isTargetProcess(api_, env_, args->nice_name))) {
            is_target_ = true;
        } else {
            api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs* args) override {
        if (LIKELY(!is_target_)) return;
        
        // Kita hook lib utama dimana Java & Native biasa memanggil System Properties
        hookLibrary(api_, "libandroid_runtime.so");
        hookLibrary(api_, "libcutils.so");
        hookLibrary(api_, "libbase.so");
        
        api_->pltHookCommit();
    }

  private:
    zygisk::Api* api_ = nullptr;
    JNIEnv* env_ = nullptr;
    bool is_target_ = false;
};

}  // namespace

REGISTER_ZYGISK_MODULE(AdbSpooferModule)