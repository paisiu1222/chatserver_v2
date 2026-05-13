#ifndef PASSWORD_UTILS_H
#define PASSWORD_UTILS_H

#include <string>

// PBKDF2-HMAC-SHA256 密码哈希工具
// 使用 OpenSSL 实现，迭代次数 100000，盐长度 16 字节
std::string hashPassword(const std::string &password);
bool verifyPassword(const std::string &password, const std::string &stored_hash);

#endif
