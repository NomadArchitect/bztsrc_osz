OS/Z Scheduler
==============

An operating system should guarantee that no task will starve ever. Linux try to solve this with dynamically
calculating a number to add to priority. OS/Z has a fundamentally different approach, namely re-entrant priority
levels. This means that one time slice at each priority level is reserved for a lower priority task.

For example if we have 3 tasks at priority 5, and no higher priority tasks are allowed to run, then the
time will be splitted into 4. Out of that 4 time slices 3 are for tasks at level 5, and one slice
is for a task at level 6 or below. Within a given priority level the task is choosen in round-rubin
style, but tasks of the same process are grouped together thus minimalizing the number of TLB cache flushes.

Priority levels
---------------

OS/Z has 8 priority levels:

| Nr. | Level | Description |
| --- | ----- | ----------- |
| 0 | PRI_SYS | System |
| 1 | PRI_RT | Real Time tasks |
| 2 | PRI_DRV | Userspace drivers |
| 3 | PRI_SRV | Services |
| 4 | PRI_APPH | Application High |
| 5 | PRI_APP | Application Normal |
| 6 | PRI_APPL | Application Low |
| 7 | PRI_IDLE | Idle |

The top priority level is PRI_SYS (zero). This queue normally is empty, and it's reserved for emergenceny tasks.

After that comes PRI_RT (1) for real time tasks. These have the highest priority a user task can own.

That's followed by PRI_DRV (2) for user space device drivers. These top 3 levels are uninterruptible,
meaning the scheduler will never pre-empt a running task in them.

The next one is PRI_SRV (3) for services aka unix daemons. This is the first time shared, pre-emptible level.

Normal applications have three levels, which allows balancing different programs. These are PRI_APPH (4, high priority)
for example video players and games, PRI_APP (5, normal priority) for example editors and browsers, and PRI_APPL
(6, low priority) for example weather widget.

Finally PRI_IDLE (7). This level is only scheduled when there're no other runnable (non-blocked) tasks at higher
priority levels. The screensaver and memory defragmenter runs at this level for example. If even this level is empty or
blocked, then a special function is scheduled in the ["IDLE" task](https://gitlab.com/bztsrc/osz/tree/master/src/core/x86_64/platform.S)
that halts the CPU.

Example
-------

Imagine the following tasks:

 - priority level 4: A, B, C
 - priority level 5: D
 - priority level 6: E, F

They will be scheduled in order:

 A B C D A B C E A B C D A B C F

All tasks had the chance to run, so there was no stravation and time was splitted up as follows:

 A: 4 times,
 B: 4 times,
 C: 4 times,
 D: 2 times,
 E: 1 time,
 F: 1 time
 
Which is pretty much the distribution one would expect for those priority levels.

Visibility
----------

The priority queue list heads are placed in the [CPU Control Block](https://gitlab.com/bztsrc/osz/tree/master/src/core/x86_64/ccb.h).
That page is per CPU core mapped, meaning all application processors will have their own priority queues.

Task States
-----------

There are three states. The following figure demostrates them and their transitions.

```
        { tcb_state_hybernated }
             ^              |
   sched_hybernate()        |
             |              |
    { tcb_state_blocked }   |
             ^      |       |
   sched_block()  sched_awake()
             |      v
      { tcb_state_running }
             ^      |
  [syscall, IRQ]  sched_pick()
             |      v
        (CPU resource allocated)
```
Running state has to sub-states: waiting to run, and actively running. From the task state's point of view this distinction
is irrelevant. When a task is actively running, it's Task Control Block is mapped at address 0.
