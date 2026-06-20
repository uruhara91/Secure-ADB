#pragma once
#include <sys/types.h>

bool getMapping(const char* library_name, ino_t* inode, dev_t* device);