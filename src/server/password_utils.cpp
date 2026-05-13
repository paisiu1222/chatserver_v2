#include "password_utils.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>
#include <cstring>

static const int SALT_LEN = 16;
static const int KEY_LEN = 32;  // SHA-256 = 32 bytes
static const int ITERATIONS = 100000;

static std::string toHex(const unsigned char *data, int len)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < len; ++i)
        oss << std::setw(2) << (int)data[i];
    return oss.str();
}

static bool fromHex(const std::string &hex, unsigned char *out)
{
    for (size_t i = 0; i < hex.length(); i += 2)
    {
        unsigned int byte;
        if (sscanf(hex.c_str() + i, "%2x", &byte) != 1)
            return false;
        out[i / 2] = (unsigned char)byte;
    }
    return true;
}

// 返回格式: hex_salt:hex_hash
std::string hashPassword(const std::string &password)
{
    unsigned char salt[SALT_LEN];
    RAND_bytes(salt, SALT_LEN);

    unsigned char key[KEY_LEN];
    PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                      salt, SALT_LEN,
                      ITERATIONS,
                      EVP_sha256(),
                      KEY_LEN, key);

    return toHex(salt, SALT_LEN) + ":" + toHex(key, KEY_LEN);
}

bool verifyPassword(const std::string &password, const std::string &stored_hash)
{
    size_t sep = stored_hash.find(':');
    if (sep == std::string::npos || sep != SALT_LEN * 2)
        return false;

    std::string salt_hex = stored_hash.substr(0, sep);
    std::string key_hex = stored_hash.substr(sep + 1);

    unsigned char salt[SALT_LEN];
    unsigned char expected_key[KEY_LEN];
    if (!fromHex(salt_hex, salt) || !fromHex(key_hex, expected_key))
        return false;

    unsigned char key[KEY_LEN];
    PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                      salt, SALT_LEN,
                      ITERATIONS,
                      EVP_sha256(),
                      KEY_LEN, key);

    return CRYPTO_memcmp(key, expected_key, KEY_LEN) == 0;
}
