/*
Copyright (c) "2018-2019", Shenzhen Mindeng Technology Co., Ltd(www.niiengine.com),
        Mindeng Base Communication Application Framework
All rights reserved.
    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
    Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice, this list
of conditions and the following disclaimer in the documentation and/or other materials
provided with the distribution.
    Neither the name of the "ORGANIZATION" nor the names of its contributors may be used
to endorse or promote products derived from this software without specific prior written
permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <stdio.h>
#include "MdfEncrypt.h"

#include <cassert>
#include <limits>
#include <stdexcept>
#include <cctype>

namespace Mdf
{

    static const char b64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    static const char reverse_table[128] =
    {
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
        64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
        64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64
    };

    String base64_encode(const String & bindata)
    {
        using std::numeric_limits;

        if (bindata.size() > (numeric_limits<String::size_type>::max() / 4u) * 3u)
        {
            //throw length_error("Converting too large a String to base64.");
            return "";
        }

        const size_t binlen = bindata.size();
        // Use = signs so the end is properly padded.
        String retval((((binlen + 2) / 3) * 4), '=');
        size_t outpos = 0;
        int bits_collected = 0;
        unsigned int accumulator = 0;
        const String::const_iterator binend = bindata.end();

        for (String::const_iterator i = bindata.begin(); i != binend; ++i)
        {
            accumulator = (accumulator << 8) | (*i & 0xffu);
            bits_collected += 8;
            while (bits_collected >= 6) {
                bits_collected -= 6;
                retval[outpos++] = b64_table[(accumulator >> bits_collected) & 0x3fu];
            }
        }
        if (bits_collected > 0) { // Any trailing bits that are missing.
            assert(bits_collected < 6);
            accumulator <<= 6 - bits_collected;
            retval[outpos++] = b64_table[accumulator & 0x3fu];
        }
        assert(outpos >= (retval.size() - 2));
        assert(outpos <= retval.size());
        return retval;
    }

    String base64_decode(const String &ascdata)
    {
        String retval;
        const String::const_iterator last = ascdata.end();
        int bits_collected = 0;
        unsigned int accumulator = 0;

        for (String::const_iterator i = ascdata.begin(); i != last; ++i)
        {
            const int c = *i;
            if (isspace(c) || c == '=')
            {
                // Skip whitespace and padding. Be liberal in what you accept.
                continue;
            }
            if ((c > 127) || (c < 0) || (reverse_table[c] > 63))
            {
                return "";
            }
            accumulator = (accumulator << 6) | reverse_table[c];
            bits_collected += 6;
            if (bits_collected >= 8)
            {
                bits_collected -= 8;
                retval += (char)((accumulator >> bits_collected) & 0xffu);
            }
        }
        return retval;
    }

    AES::AES(const String & strKey)
    {
        AES_set_encrypt_key((const unsigned char*)strKey.c_str(), 256, &m_cEncKey);
        AES_set_decrypt_key((const unsigned char*)strKey.c_str(), 256, &m_cDecKey);
    }

    int AES::Encrypt(const char* pInData, uint32_t nInLen, char** ppOutData, uint32_t& nOutLen)
    {
        if (pInData == NULL || nInLen <= 0)
        {
            return -1;
        }
        uint32_t nRemain = nInLen % 16;
        uint32_t nBlocks = (nInLen + 15) / 16;

        if (nRemain > 12 || nRemain == 0) {
            nBlocks += 1;
        }
        uint32_t nEncryptLen = nBlocks * 16;

        unsigned char* pData = (unsigned char*)calloc(nEncryptLen, 1);
        memcpy(pData, pInData, nInLen);
        unsigned char* pEncData = (unsigned char*)malloc(nEncryptLen);

        MemStream::write((pData + nEncryptLen - 4), nInLen);
        for (uint32_t i = 0; i < nBlocks; i++)
        {
            AES_encrypt(pData + i * 16, pEncData + i * 16, &m_cEncKey);
        }

        free(pData);
        String strEnc((char*)pEncData, nEncryptLen);
        free(pEncData);
        String strDec = base64_encode(strEnc);
        nOutLen = (uint32_t)strDec.length();

        char* pTmp = (char*)malloc(nOutLen + 1);
        memcpy(pTmp, strDec.c_str(), nOutLen);
        pTmp[nOutLen] = 0;
        *ppOutData = pTmp;
        return 0;
    }

    int AES::Decrypt(const char* pInData, uint32_t nInLen, char** ppOutData, uint32_t& nOutLen)
    {
        if (pInData == NULL || nInLen <= 0)
        {
            return -1;
        }
        String strInData(pInData, nInLen);
        String strResult = base64_decode(strInData);
        uint32_t nLen = (uint32_t)strResult.length();
        if (nLen == 0)
        {
            return -2;
        }

        const unsigned char* pData = (const unsigned char*)strResult.c_str();

        if (nLen % 16 != 0)
        {
            return -3;
        }
        // 先申请nLen 个长度，解密完成后的长度应该小于该长度
        char* pTmp = (char*)malloc(nLen + 1);

        uint32_t nBlocks = nLen / 16;
        for (uint32_t i = 0; i < nBlocks; i++)
        {
            AES_decrypt(pData + i * 16, (unsigned char*)pTmp + i * 16, &m_cDecKey);
        }

        uchar_t* pStart = (uchar_t*)pTmp + nLen - 4;
        MemStream::read(pStart, nOutLen);
        //        printf("%u\n", nOutLen);
        if (nOutLen > nLen)
        {
            free(pTmp);
            return -4;
        }
        pTmp[nOutLen] = 0;
        *ppOutData = pTmp;
        return 0;
    }

    void AES::Free(char * pOutData)
    {
        if (pOutData)
        {
            free(pOutData);
            pOutData = NULL;
        }
    }

    void MD5::calc(const char *pContent, unsigned int nLen, char *out)
    {
        uchar_t d[16];
        MD5_CTX ctx;
        MD5_Init(&ctx);
        MD5_Update(&ctx, pContent, nLen);
        MD5_Final(d, &ctx);
        for (int i = 0; i < 16; ++i)
        {
            snprintf(out + (i * 2), 32, "%02x", d[i]);
        }
        out[32] = 0;
        return;
    }

#define AUTH_ENCRYPT_KEY "Mgj!@#123"

    // Constants are the integer part of the sines of integers (in radians) * 2^32.
    const uint32_t k[64] =
    {
        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee ,
        0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501 ,
        0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be ,
        0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821 ,
        0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa ,
        0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8 ,
        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed ,
        0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a ,
        0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c ,
        0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70 ,
        0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05 ,
        0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665 ,
        0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039 ,
        0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1 ,
        0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1 ,
        0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
    };

    // r specifies the per-round shift amounts
    const uint32_t r[] =
    {
        7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
        5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
        4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
        6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
    };

    // leftrotate function definition
#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))

    void to_bytes(uint32_t val, uint8_t *bytes)
    {
        bytes[0] = (uint8_t)val;
        bytes[1] = (uint8_t)(val >> 8);
        bytes[2] = (uint8_t)(val >> 16);
        bytes[3] = (uint8_t)(val >> 24);
    }

    uint32_t to_int32(const uint8_t * bytes)
    {
        return (uint32_t)bytes[0]
            | ((uint32_t)bytes[1] << 8)
            | ((uint32_t)bytes[2] << 16)
            | ((uint32_t)bytes[3] << 24);
    }

    void md5(const uint8_t * initial_msg, size_t initial_len, uint8_t * digest)
    {

        // These vars will contain the hash
        uint32_t h0, h1, h2, h3;

        // Message (to prepare)
        uint8_t * msg = NULL;

        size_t new_len, offset;
        uint32_t w[16];
        uint32_t a, b, c, d, i, f, g, temp;

        // Initialize variables - simple count in nibbles:
        h0 = 0x67452301;
        h1 = 0xefcdab89;
        h2 = 0x98badcfe;
        h3 = 0x10325476;

        //Pre-processing:
        //append "1" bit to message
        //append "0" bits until message length in bits ≡ 448 (mod 512)
        //append length mod (2^64) to message

        for (new_len = initial_len + 1; new_len % (512 / 8) != 448 / 8; new_len++)
            ;

        msg = (uint8_t*)malloc(new_len + 8);
        memcpy(msg, initial_msg, initial_len);
        msg[initial_len] = 0x80; // append the "1" bit; most significant bit is "first"
        for (offset = initial_len + 1; offset < new_len; offset++)
            msg[offset] = 0; // append "0" bits

        // append the len in bits at the end of the buffer.
        to_bytes(initial_len * 8, msg + new_len);
        // initial_len>>29 == initial_len*8>>32, but avoids overflow.
        to_bytes(initial_len >> 29, msg + new_len + 4);

        // Process the message in successive 512-bit chunks:
        //for each 512-bit chunk of message:
        for (offset = 0; offset < new_len; offset += (512 / 8))
        {

            // break chunk into sixteen 32-bit words w[j], 0 ≤ j ≤ 15
            for (i = 0; i < 16; i++)
                w[i] = to_int32(msg + offset + i * 4);

            // Initialize hash value for this chunk:
            a = h0;
            b = h1;
            c = h2;
            d = h3;

            // Main loop:
            for (i = 0; i < 64; i++)
            {

                if (i < 16)
                {
                    f = (b & c) | ((~b) & d);
                    g = i;
                }
                else if (i < 32)
                {
                    f = (d & b) | ((~d) & c);
                    g = (5 * i + 1) % 16;
                }
                else if (i < 48)
                {
                    f = b ^ c ^ d;
                    g = (3 * i + 5) % 16;
                }
                else
                {
                    f = c ^ (b | (~d));
                    g = (7 * i) % 16;
                }

                temp = d;
                d = c;
                c = b;
                b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
                a = temp;

            }

            // Add this chunk's hash to result so far:
            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;

        }
        free(msg);

        //var char digest[16] := h0 append h1 append h2 append h3 //(Output is in little-endian)
        to_bytes(h0, digest);
        to_bytes(h1, digest + 4);
        to_bytes(h2, digest + 8);
        to_bytes(h3, digest + 12);
    }

    int genToken(unsigned int uid, time_t time_offset, char* md5_str_buf)
    {
        //MD5_CTX ctx;
        char tmp_buf[256];
        char t_buf[128];
        unsigned char md5_buf[32];
#ifdef _WIN32
        SYSTEMTIME systemTime;
        GetLocalTime(&systemTime);
        snprintf(t_buf, sizeof(t_buf), "%04d-%02d-%02d-%02d", systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour);
#else
        struct tm* tm;
        time_t currTime;
        time(&currTime);
        currTime += time_offset;
        tm = localtime(&currTime);
        snprintf(t_buf, sizeof(t_buf), "%04d-%02d-%02d-%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour);
#endif
        snprintf(tmp_buf, sizeof(tmp_buf), "%s_%u_%s_%s", AUTH_ENCRYPT_KEY, uid, t_buf, AUTH_ENCRYPT_KEY);

        md5((unsigned char*)tmp_buf, strlen(tmp_buf), md5_buf);

        for (int i = 0; i < 16; i++)
        {
            sprintf(md5_str_buf + 2 * i, "%02x", md5_buf[i]);
        }

        // reverse md5_str_buf
        char c = 0;
        for (int i = 0; i < 16; i++)
        {
            c = md5_str_buf[i];
            md5_str_buf[i] = md5_str_buf[31 - i];
            md5_str_buf[31 - i] = c;
        }

        // switch md5_str_buf[i] and md5_str_buf[i + 1]
        for (int i = 0; i < 32; i += 2)
        {
            c = md5_str_buf[i];
            md5_str_buf[i] = md5_str_buf[i + 1];
            md5_str_buf[i + 1] = c;
        }

        return 0;
    }

    bool IsTokenValid(uint32_t user_id, const char* token)
    {
        char token1[64], token2[64], token3[64];

        genToken(user_id, -3600, token1);    // token an hour ago
        genToken(user_id, 0, token2);        // current token
        genToken(user_id, 3600, token3);    // token an hour later

        if (!strcmp(token, token1) || !strcmp(token, token2) || !strcmp(token, token3))
        {
            return true;
        }
        return false;
    }
}