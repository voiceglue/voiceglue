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

#include "base64.h"

#define _ASSERTE(exp) if ( !(exp) ) return -1
#define _CrtIsValidPointer(ptr, len, x) (((ptr) != 0) && ((len) > 0))

const char _b64_etbl[64] = {
    'A','B','C','D',  'E','F','G','H',	'I','J','K','L',  'M','N','O','P',
    'Q','R','S','T',  'U','V','W','X',	'Y','Z','a','b',  'c','d','e','f',
    'g','h','i','j',  'k','l','m','n',	'o','p','q','r',  's','t','u','v',
    'w','x','y','z',  '0','1','2','3',	'4','5','6','7',  '8','9','+','/'
};

const char _b64_dtbl[128] = { 
    0x40,0x40,0x40,0x40,  0x40,0x40,0x40,0x40,	0x40,0x40,0x40,0x40,  0x40,0x40,0x40,0x40,
    0x40,0x40,0x40,0x40,  0x40,0x40,0x40,0x40,	0x40,0x40,0x40,0x40,  0x40,0x40,0x40,0x40,
    0x40,0x40,0x40,0x40,  0x40,0x40,0x40,0x40,	0x40,0x40,0x40,0x3E,  0x40,0x40,0x40,0x3F,
    0x34,0x35,0x36,0x37,  0x38,0x39,0x3A,0x3B,	0x3C,0x3D,0x40,0x40,  0x40,0x00,0x40,0x40,
    
    0x40,0x00,0x01,0x02,  0x03,0x04,0x05,0x06,	0x07,0x08,0x09,0x0A,  0x0B,0x0C,0x0D,0x0E,
    0x0F,0x10,0x11,0x12,  0x13,0x14,0x15,0x16,	0x17,0x18,0x19,0x40,  0x40,0x40,0x40,0x40,
    0x40,0x1A,0x1B,0x1C,  0x1D,0x1E,0x1F,0x20,	0x21,0x22,0x23,0x24,  0x25,0x26,0x27,0x28,
    0x29,0x2A,0x2B,0x2C,  0x2D,0x2E,0x2F,0x30,	0x31,0x32,0x33,0x40,  0x40,0x40,0x40,0x40
};

//---------------------------------------------------------------------------
// b64elen
//
//  Calculates length of base64-encoded string given size of binary data.
//
//  Parameters:
//    cb - number of bytes of binary data
//
//  Returns:
//    the length of base64-encoded string not including the terminating null
//    character.
//
extern "C" long
b64elen(
    size_t cb
    )
{
    return (cb + 2) / 3 * 4;
}

//---------------------------------------------------------------------------
// b64dlen
//
//  Calculates size of binary data which will be produced after decoding of 
//  base64-encoded string with specified length.
//
//  Parameters:
//    cch - length of the string, not including terminating null character
//
//  Returns:
//    the number of bytes of binary data.
//
extern "C" long
b64dlen(
    size_t cch
    )
{
    return cch * 3 / 4 + (cch % 4); 
}

//---------------------------------------------------------------------------
// strb64e
//
//  Converts a buffer with binary data into a base64-encoded string. The
//  function does not store a null character at the end of the string.
//
//  Parameters:
//    p   - pointer to a buffer with binary data
//    cb  - size of the buffer in bytes
//    psz - pointer to a buffer that receives encoded string
//
//  Returns:
//    number of characters written into the output buffer.
//
//  Note:
//    Required size of the output buffer can be determined as b64elen(cb).
//
extern "C" long
strb64e(
    const void * p, 
    size_t cb, 
    char * psz
    )
{
    _ASSERTE(_CrtIsValidPointer(p, cb, 0));
    _ASSERTE(_CrtIsValidPointer(psz, b64elen(cb), 1));

    int i;
    int rem = cb % 3;
    int cbr = cb - rem;
    _ASSERTE((cbr % 3) == 0);

    char * qsz = psz;
    const char * pb = (const char *)p;
    register int dw;

    // convert each three consequent bytes into four base64 characters
    for (i = 0; i < cbr; i += 3, psz += 4)
    {
	dw = ((unsigned char)pb[i]) << 16;
	dw |= ((unsigned char)pb[i + 1]) << 8;
	dw |= ((unsigned char)pb[i + 2]);

	psz[3] = _b64_etbl[dw & 0x3F];	dw >>= 6;
	psz[2] = _b64_etbl[dw & 0x3F];	dw >>= 6;
	psz[1] = _b64_etbl[dw & 0x3F];	dw >>= 6;
	psz[0] = _b64_etbl[dw & 0x3F];
    }

    // 0, 1, or 2 bytes can remain at the end of the input buffer; deal
    // with each of these cases separately
    switch (rem)
    {
	case 2:
	    dw = ((unsigned char)pb[i]) << 8;
	    dw |= ((unsigned char)pb[i + 1]);

	    psz[3] = '=';
	    psz[2] = _b64_etbl[dw & 0x3F];  dw >>= 6;
	    psz[1] = _b64_etbl[dw & 0x3F];  dw >>= 6;
	    psz[0] = _b64_etbl[dw & 0x3F];
	    return qsz - psz + 4;

	case 1:
	    dw = ((unsigned char)pb[i]);

	    psz[3] = '=';
	    psz[2] = '=';
	    psz[1] = _b64_etbl[dw & 0x3F];  dw >>= 6;
	    psz[0] = _b64_etbl[dw & 0x3F];
	    return qsz - psz + 4;

	case 0:
	    return qsz - psz;

	default:
	    _ASSERTE(0);
	    // __assume(0);
    }
}

//---------------------------------------------------------------------------
// wcsb64e
//
//  Converts a buffer with binary data into a base64-encoded string. The
//  function does not store a null character at the end of the string.
//
//  Parameters:
//    p   - pointer to a buffer with binary data
//    cb  - size of the buffer in bytes
//    psz - pointer to a buffer that receives encoded string
//
//  Returns:
//    number of characters written into the output buffer.
//
//  Note:
//    Required size of the output buffer can be determined as b64elen(cb).
//
extern "C" long
wcsb64e(
    const void * p, 
    size_t cb, 
    wchar_t * psz
    )
{
    _ASSERTE(_CrtIsValidPointer(p, cb, 0));
    _ASSERTE(_CrtIsValidPointer(psz, b64elen(cb) * sizeof(wchar_t), 1));

    int i;
    int rem = cb % 3;
    int cbr = cb - rem;
    _ASSERTE((cbr % 3) == 0);

    wchar_t * qsz = psz;
    const char * pb = (const char *)p;
    register int dw;

    // convert each three consequent bytes into four base64 characters
    for (i = 0; i < cbr; i += 3, psz += 4)
    {
	dw = ((unsigned char)pb[i]) << 16;
	dw |= ((unsigned char)pb[i + 1]) << 8;
	dw |= ((unsigned char)pb[i + 2]);

	psz[3] = (wchar_t)_b64_etbl[dw & 0x3F]; dw >>= 6;
	psz[2] = (wchar_t)_b64_etbl[dw & 0x3F]; dw >>= 6;
	psz[1] = (wchar_t)_b64_etbl[dw & 0x3F]; dw >>= 6;
	psz[0] = (wchar_t)_b64_etbl[dw & 0x3F];
    }

    // 0, 1, or 2 bytes can remain at the end of the input buffer; deal
    // with each of these cases separately
    switch (rem)
    {
	case 2:
	    dw = ((unsigned char)pb[i]) << 8;
	    dw |= ((unsigned char)pb[i + 1]);

	    psz[3] = L'=';
	    psz[2] = (wchar_t)_b64_etbl[dw & 0x3F]; dw >>= 6;
	    psz[1] = (wchar_t)_b64_etbl[dw & 0x3F]; dw >>= 6;
	    psz[0] = (wchar_t)_b64_etbl[dw & 0x3F];
	    return qsz - psz + 4;

	case 1:
	    dw = ((unsigned char)pb[i]);

	    psz[3] = L'=';
	    psz[2] = L'=';
	    psz[1] = (wchar_t)_b64_etbl[dw & 0x3F]; dw >>= 6;
	    psz[0] = (wchar_t)_b64_etbl[dw & 0x3F];
	    return qsz - psz + 4;

	case 0:
	    return qsz - psz;

	default:
	    _ASSERTE(0);
	    // __assume(0);
    }
}

//---------------------------------------------------------------------------
// strb64d
//
//  Converts a base64-encoded string into binary data. If the input string
//  contains characters that are not allowed for base64, the contents of the
//  output buffer is unpredictable.
//
//  Parameters:
//    psz - pointer to a base64-encoded sting
//    cch - number of characters in the string
//    p   - pointer to a buffer that receives binary data
//
//  Returns:
//    number of bytes stored into the output buffer.
//
//  Note:
//    Required size of the output buffer can be determined as b64dlen(cch).
//
extern "C" long
strb64d(
    const char * psz, 
    size_t cch, 
    void * p
    )
{
    _ASSERTE(_CrtIsValidPointer(psz, cch, 0));
    _ASSERTE(_CrtIsValidPointer(p, b64dlen(cch), 1));
    int i;

    // ignore '=' characters that might appear at the end of the string
    while (cch > 0 && psz[cch - 1] == '=')
	cch--;

    int rem = cch & 3;
    int cch4 = cch - rem;

    char * pb = (char *)p;  
    register int dw;

    // convert each four consequtive base64 characters into three bytes
    // of binary data
    for (i = 0; i < cch4; i += 4, pb += 3)
    {
	dw = _b64_dtbl[0x7F & psz[i]] << 18;
	dw |= _b64_dtbl[0x7F & psz[i + 1]] << 12;
	dw |= _b64_dtbl[0x7F & psz[i + 2]] << 6;
	dw |= _b64_dtbl[0x7F & psz[i + 3]];

	pb[2] = (unsigned char)dw;  dw >>= 8;
	pb[1] = (unsigned char)dw;  dw >>= 8;
	pb[0] = (unsigned char)dw;
    }

    // a few characters might remain at the end of the string
    switch (rem)
    {
	case 3:
	    dw = _b64_dtbl[0x7F & psz[i]] << 12;
	    dw |= _b64_dtbl[0x7F & psz[i + 1]] << 6;
	    dw |= _b64_dtbl[0x7F & psz[i + 2]];

	    //pb[2] = (unsigned char)dw;    dw >>= 8;
	    pb[1] = (unsigned char)dw;	dw >>= 8;
	    pb[0] = (unsigned char)dw;
	    return pb - (char *)p + 2;

	case 2:
	    dw = _b64_dtbl[0x7F & psz[i]] << 6;
	    dw |= _b64_dtbl[0x7F & psz[i + 1]];

	    //pb[1] = (unsigned char)dw;    dw >>= 8;
	    pb[0] = (unsigned char)dw;
	    return pb - (char *)p + 1;

	case 1:
	    //pb[0] = _b64_dtbl[0x7F & psz[i]];
	    return pb - (char *)p;

	case 0:
	    return pb - (char *)p;

	default:
	    _ASSERTE(0);
	    // __assume(0);
    }
}

//---------------------------------------------------------------------------
// wcsb64d
//
//  Converts a base64-encoded string into binary data. If the input string
//  contains characters that are not allowed for base64, the contents of the
//  output buffer is unpredictable.
//
//  Parameters:
//    psz - pointer to a base64-encoded sting
//    cch - number of characters in the string
//    p   - pointer to a buffer that receives binary data
//
//  Returns:
//    number of bytes written into the output buffer.
//
//  Note:
//    Required size of the output buffer can be determined as b64dlen(cch).
//
extern "C" long
wcsb64d(
    const wchar_t * psz, 
    size_t cch, 
    void * p
    )
{
    _ASSERTE(_CrtIsValidPointer(psz, cch * sizeof(wchar_t), 0));
    _ASSERTE(_CrtIsValidPointer(p, b64dlen(cch), 1));
    int i;

    // ignore '=' characters that might appear at the end of the string
    while (cch > 0 && psz[cch - 1] == L'=')
	cch--;

    int rem = cch & 3;
    int cch4 = cch - rem;

    char * pb = (char *)p;  
    register int dw;

    // convert each four consequtive base64 characters into three bytes
    // of binary data
    for (i = 0; i < cch4; i += 4, pb += 3)
    {
	dw = _b64_dtbl[0x7F & psz[i]] << 18;
	dw |= _b64_dtbl[0x7F & psz[i + 1]] << 12;
	dw |= _b64_dtbl[0x7F & psz[i + 2]] << 6;
	dw |= _b64_dtbl[0x7F & psz[i + 3]];

	pb[2] = (unsigned char)dw;  dw >>= 8;
	pb[1] = (unsigned char)dw;  dw >>= 8;
	pb[0] = (unsigned char)dw;
    }

    // a few characters might remain at the end of the string
    switch (rem)
    {
	case 3:
	    dw = _b64_dtbl[0x7F & psz[i]] << 12;
	    dw |= _b64_dtbl[0x7F & psz[i + 1]] << 6;
	    dw |= _b64_dtbl[0x7F & psz[i + 2]];

	    //pb[2] = (unsigned char)dw;    dw >>= 8;
	    pb[1] = (unsigned char)dw;	dw >>= 8;
	    pb[0] = (unsigned char)dw;
	    return pb - (char *)p + 2;

	case 2:
	    dw = _b64_dtbl[0x7F & psz[i]] << 6;
	    dw |= _b64_dtbl[0x7F & psz[i + 1]];

	    //pb[1] = (unsigned char)dw;    dw >>= 8;
	    pb[0] = (unsigned char)dw;
	    return pb - (char *)p + 1;

	case 1:
	    //pb[0] = _b64_dtbl[0x7F & psz[i]];
	    return pb - (char *)p;

	case 0:
	    return pb - (char *)p;

	default:
	    _ASSERTE(0);
	    // __assume(0);
    }
}

//---------------------------------------------------------------------------
// strisb64
//
//  Determines whether a string is a valid base64-encoded string.
//
//  Parameters:
//    psz - pointer to a string
//    cch - number of characters in the string
//
//  Returns:
//    nonzero, if the string entirely consists of valid base64 characters,
//    zero - otherwise.
//
extern "C" int
strisb64(
    const char * psz,
    size_t cch
    )
{
    _ASSERTE(_CrtIsValidPointer(psz, cch * sizeof(char), 0));
    size_t i;

    for (i = 0; (i < cch) && (psz[i] != '='); i++)
    {
	if ((psz[i] & 0x80) != 0)
	    return 0;
	if (_b64_dtbl[psz[i]] == 0x40)
	    return 0;
    }

    if (i == cch)
	return 1;

    if ((cch % 4) != 0)
	return 0;

    switch (cch - i)
    {
	case 2:
	    if (psz[i + 1] != '=')
		return 0;
	case 1:
	case 0:
	    break;

	default:
	    return 0;
    }

    return 1;
}

//---------------------------------------------------------------------------
// wcsisb64
//
//  Determines whether a string is a valid base64-encoded string.
//
//  Parameters:
//    psz - pointer to a string
//    cch - number of characters in the string
//
//  Returns:
//    nonzero, if the string entirely consists of valid base64 characters,
//    zero - otherwise.
//
extern "C" int
wcsisb64(
    const wchar_t * psz,
    size_t cch
    )
{
    _ASSERTE(_CrtIsValidPointer(psz, cch * sizeof(wchar_t), 0));
    size_t i;

    for (i = 0; (i < cch) && (psz[i] != L'='); i++)
    {
	if ((psz[i] & 0xFF80) != 0)
	    return 0;
	if (_b64_dtbl[psz[i]] == 0x40)
	    return 0;
    }

    if (i == cch)
	return 1;

    if ((cch % 4) != 0)
	return 0;

    switch (cch - i)
    {
	case 2:
	    if (psz[i + 1] != L'=')
		return 0;
	case 1:
	case 0:
	    break;

	default:
	    return 0;
    }

    return 1;
}
