#pragma once

#include "../util/read_write.h"

#define rcu_assign_pointer(value, new)      WRITE_ONCE(value, READ_ONCE(new))
#define rcu_dereference(p)                  READ_ONCE(p)

void rcu_read_lock();
void rcu_read_unlock();
void synchronize_rcu();
