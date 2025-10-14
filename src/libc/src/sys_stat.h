#pragma once

mode_t umask(mode_t mask)
{
    mode_t ret = fd_creation_mask;
    fd_creation_mask = mask & 0777;
    return ret;
}