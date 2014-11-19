/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#ifdef NULL
#undef NULL
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "ntapi.h"

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

int main(int _Argc, char **_Argv, char **_Env);
int wmain(int _Argc, wchar_t **_Argv, wchar_t **_Env);

#ifdef _WIN64
#define DEFAULT_SECURITY_COOKIE 0x00002B992DDFA232ll
#else
#define DEFAULT_SECURITY_COOKIE 0xBB40E64E
#endif
DECLSPEC_SELECTANY UINT_PTR __security_cookie = DEFAULT_SECURITY_COOKIE;
DECLSPEC_SELECTANY UINT_PTR __security_cookie_complement = ~(DEFAULT_SECURITY_COOKIE);

static void __security_init_cookie (void)
{
  UINT_PTR cookie;
  LARGE_INTEGER perfctr, perffreq;

  if (__security_cookie != DEFAULT_SECURITY_COOKIE) {
      __security_cookie_complement = ~__security_cookie;
      return;
  }

  NtQueryPerformanceCounter(&perfctr, &perffreq);
#ifdef _WIN64
  cookie = perfctr.QuadPart;
#else
  cookie = perfctr.LowPart;
  cookie ^= perfctr.HighPart;
#endif

  if (cookie == DEFAULT_SECURITY_COOKIE)
    cookie = DEFAULT_SECURITY_COOKIE + 1;
  __security_cookie = cookie;
  __security_cookie_complement = ~cookie;
}

static int wargc;
static wchar_t **wargv;

static WCHAR** _CommandLineToArgvW(UNICODE_STRING *CmdLine, int* _argc)
{
    WCHAR   **argv;
    WCHAR   *_argv, *argv_alloc = NULL;
    WCHAR   a;
    ULONG   len, argc, i = 0, j = 0;
    SIZE_T  k;
    BOOLEAN  in_QM = FALSE, in_TEXT = FALSE, in_SPACE = TRUE;

    len = CmdLine->Length;
    i = ((len+2)/2)*sizeof(VOID*) + sizeof(VOID*);
    k = (i + (len+2)*sizeof(WCHAR));
    NtAllocateVirtualMemory((HANDLE)-1, (void**)&argv_alloc, 0, &k, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    argv = (WCHAR**)argv_alloc;
    _argv = (WCHAR*)(((UCHAR*)argv)+i);

    argc = 0;
    argv[argc] = _argv;
    i = 0;

    while((a = CmdLine->Buffer[i])) {
        if(in_QM) {
            if(a == L'\"') {
                in_QM = FALSE;
            } else {
                _argv[j] = a;
                j++;
            }
        } else {
            switch(a) {
            case L'\"':
                in_QM = TRUE;
                in_TEXT = TRUE;
                if(in_SPACE) {
                    argv[argc] = _argv+j;
                    argc++;
                }
                in_SPACE = FALSE;
                break;
            case L' ':
            case L'\t':
            case L'\n':
            case L'\r':
                if(in_TEXT) {
                    _argv[j] = '\0';
                    j++;
                }
                in_TEXT = FALSE;
                in_SPACE = TRUE;
                break;
            default:
                in_TEXT = TRUE;
                if(in_SPACE) {
                    argv[argc] = _argv+j;
                    argc++;
                }
                _argv[j] = a;
                j++;
                in_SPACE = FALSE;
                break;
            }
        }
        i++;
    }
    _argv[j] = L'\0';
    argv[argc] = NULL;

    (*_argc) = argc;
    return argv;
}

#ifdef COMPILE_NATIVE_BINARY
int NtProcessStartup(void) {
#else
int mainCRTStartup (void)
#endif
{
  int ret = 255;
  UNICODE_STRING wcmdln;
  RtlGetCommandLine(&wcmdln);

#if defined(__i386__) && !defined(__x86_64__)
    /*
     * Initialize floating point unit.
     */
    __cpu_features_init ();   /* Do we have SSE, etc.*/
    _fpreset ();          /* Supplied by the runtime library. */
#endif
  __security_init_cookie ();
  wargv = _CommandLineToArgvW(&wcmdln, &wargc);
#if defined(__i386__) && !defined(__x86_64__)
    /* Align the stack to 16 bytes for the sake of SSE ops in main
     * or in functions inlined into main.
     */
    __asm__ __volatile__  ("andl $-16, %%esp" ::: "%esp");
#endif
  ret = wmain(wargc, wargv, NULL);
  NtTerminateProcess(NtCurrentProcess(), ret);
  return ret;
}

