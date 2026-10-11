#pragma once

typedef struct mountpoint mountpoint_t;

typedef struct mountpoint_ref 
{
    mountpoint_t* _Atomic ptr;
} mountpoint_ref_t;
