#pragma once

// Just to make clangd shut up
#ifndef __clang__
#define lambda(type, args) ({ type ___f args; ___f; })
#else
#define lambda(type, args) NULL
#endif
