#include "poll.h"

#include "../multitasking/multitasking.h"

void __task_stop_polling(thread_t *task)
{
    ll_item_t* it = task->_poll_tqs;
    if (it)
    {
        do
        {
            ll_item_t* cur = it;
            thread_queue_t* tq = cur->data;
            assert(tq);
            it = it->next;
            __thread_queue_remove(tq, ll_find_item_by_data(tq, task));
            ll_remove(&task->_poll_tqs, cur);
        } while (task->_poll_tqs && it != task->_poll_tqs);
    }
    __thread_queue_remove(&_waiting_for_stdin_tasks, ll_find_item_by_data(&_waiting_for_stdin_tasks, task));
    if (task->queue == &waiting_for_time_tasks)
        __move_task_to_queue(&running_tasks, task);
}
