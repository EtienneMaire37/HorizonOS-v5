#pragma once

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef long long ptrdiff_t;
typedef unsigned long long size_t;
typedef char wchar_t;

#define offsetof(st, m) ((size_t)&(((st*)0)->m))