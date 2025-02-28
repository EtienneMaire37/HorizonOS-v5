#pragma once

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef unsigned int size_t;
typedef long long ptrdiff_t;
typedef char wchar_t;

#define offsetof(st, m) ((size_t)&(((st *)0)->m))