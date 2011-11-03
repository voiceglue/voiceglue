/*****************************************************************************
 *****************************************************************************
 *
 *
 * Base64 conversion.
 *
 * Written by Alex Fedotov, posted at CodeGuru
 * (http://codeguru.earthweb.com) with no copyright or restrictions on
 * its use.  
 *
 *****************************************************************************
 ****************************************************************************/

#ifndef __base64_h_included
#define __base64_h_included

#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#define B64ELEN(cb)  (((cb) + 2) / 3 * 4)
#define B64DLEN(cch) ((cch) * 3 / 4 + ((cch) % 4))

long b64elen(size_t cb);
long b64dlen(size_t cch);

long strb64d(const char * psz, size_t cch, void * p);
long strb64e(const void * p, size_t cb, char * psz);
int strisb64(const char * psz, size_t cch);

long wcsb64d(const wchar_t * psz, size_t cch, void * p);
long wcsb64e(const void * p, size_t cb, wchar_t * psz);
int wcsisb64(const wchar_t * psz, size_t cch);

#ifdef __cplusplus
}
#endif

#endif // __base64_h_included
