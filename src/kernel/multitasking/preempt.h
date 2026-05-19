#pragma once

extern int _Atomic preempt_disable_depth;

void recursive_preempt_disable();
void recursive_preempt_enable();
