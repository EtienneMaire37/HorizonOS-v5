#pragma once

uint32_t cpuid_highest_function_parameter = 0xffffffff, cpuid_highest_extended_function_parameter = 0xffffffff;
bool has_cpuid;

#define cpuid(eax, return_eax, return_ebx, return_ecx, return_edx)  if (!has_cpuid) LOG(ERROR, "CPUID not supported"); \
                                                                    else if (cpuid_highest_function_parameter >= eax || ((eax & 0x80000000) && cpuid_highest_extended_function_parameter >= eax)) \
                                                                        asm volatile("cpuid" : "=a" (return_eax), "=b" (return_ebx), "=c" (return_ecx), "=d" (return_edx) : "a" (eax)); \
                                                                    else \
                                                                        LOG(ERROR, "CPUID function not supported (0x%x)", eax);

char manufacturer_id_string[13] = { 0 };    // 12 byte string ending with ASCII NULL