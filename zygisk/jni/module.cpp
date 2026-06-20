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
    if (UNLIKELY(name != nullptr)) {
        if (UNLIKELY(strcmp(name, "sys.usb.config") == 0) ||
            UNLIKELY(strcmp(name, "persist.sys.usb.config") == 0)) {
            if (value) strcpy(value, "mtp");
            return 3;
        }
    }
    return g_orig_system_property_get(name, value);
}

COLD void hookLibrary(zygisk::Api* api, const char* lib_name) {
    ino_t inode = 0;
    dev_t device = 0;
    if (getMapping(lib_name, &inode, &device)) {
        api->pltHookRegister(device, inode, "__system_property_get",
                             reinterpret_cast<void*>(&my_system_property_get),
                             reinterpret_cast<void**>(&g_orig_system_property_get));
    }
}

COLD bool isTargetProcess(zygisk::Api* api, JNIEnv* env, jstring nice_name) {
    if (!nice_name) return false;
    const char* process_name = env->GetStringUTFChars(nice_name, nullptr);
    if (!process_name) return false;

    bool is_target = false;
    int dir_fd = api->getModuleDir();
    if (dir_fd >= 0) {
        int fd = openat(dir_fd, "target.txt", O_RDONLY);
        if (fd >= 0) {
            char buf[4096];
            ssize_t bytes = read(fd, buf, sizeof(buf) - 1);
            if (bytes > 0) {
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
        close(dir_fd);
    }
    env->ReleaseStringUTFChars(nice_name, process_name);
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

}

REGISTER_ZYGISK_MODULE(AdbSpooferModule)