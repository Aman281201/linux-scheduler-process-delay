# Linux Kernel Experiments

This repo has two experiments I did on Linux kernel 5.14.3 as part of my Operating Systems course. In both of them I have added new system calls to the kernel. The main one is a change in the CFS scheduler and the second one is a producer consumer queue which lives inside the kernel (this one is not complete yet).

## 1. CFS process delay

Folder: [`cfs-process-delay/`](./cfs-process-delay)

### Little background on CFS

CFS (Completely Fair Scheduler) was the default Linux scheduler from 2.6.23 till 6.6, after that it got replaced by EEVDF. In CFS every task has a value called `vruntime` (virtual runtime, in ns). All the runnable tasks of one CPU are kept in a red black tree sorted on `vruntime` and the scheduler always picks the leftmost task, i.e. the task which has the smallest `vruntime`. While a task is running its `vruntime` keeps increasing, so after some time some other task becomes leftmost and gets the CPU. Nice value comes into the picture through weight, a task with lower nice value has its `vruntime` increasing slowly so it gets more CPU.

### What I changed

I added a new field `delayAdd` in `struct sched_entity` and a system call `higgsCall_delay(pid, delay)` which sets this field for any process. Then in `update_curr()` in `kernel/sched/fair.c` I am adding `delayAdd` to `vruntime` along with the normal increment:

```
curr->vruntime += calc_delta_fair(delta_exec, curr) + curr->delayAdd;
```

So a process with a delay set looks like it has run much more than it actually did. It moves to the right side of the tree and other tasks on that CPU get picked before it.

### The Idea
 
1. If the delayed process is alone on a CPU it runs at full speed. There is no one else to pick so the high `vruntime` does not matter.
2. The delay is not added per ms. It gets added every time `update_curr()` runs for that task, and `update_curr()` is called on every scheduler tick and also on enqueue, dequeue, wakeup preemption check, yield etc. So the actual penalty depends on how busy the system is.
3. With a big value the process almost stops whenever a normal process is waiting on the same CPU, and then runs fully once that CPU is free. So it behaves more like a very low priority task (close to `SCHED_IDLE`) and not like a slightly higher nice value.
4. `vruntime` is only compared inside one CPU's run queue. The load balancer looks at weight and not at `vruntime`, so on a multi core machine a delayed process can still finish before some normal processes.

Full details, patches and test are in [`cfs-process-delay/`](./cfs-process-delay).

## 2. Producer consumer using syscalls

Folder: [`producer-consumer-syscall/`](./producer-consumer-syscall)

Here I added three syscalls which manage a FIFO queue kept inside the kernel as a linked list in `sys.c`, with a producer and a consumer program in user space.

| Number | Syscall | Work |
|--------|---------|------|
| 449 | `higgsQueue()` | initialise the queue |
| 450 | `writeCall(buf)` | add 8 bytes from user space |
| 451 | `readCall()` | remove from head and print using `printk` |

More details in [`producer-consumer-syscall/`](./producer-consumer-syscall).

## Syscall numbers

| Number | Name | Experiment |
|--------|------|------------|
| 448 | `higgsCall_delay` | cfs-process-delay |
| 449 | `higgsQueue` | producer-consumer-syscall |
| 450 | `writeCall` | producer-consumer-syscall |
| 451 | `readCall` | producer-consumer-syscall |

Note: the patches also have one more syscall `kernel_2d_memcpy` (335). It is from an earlier assignment and is not part of these experiments.

## Common setup

1. Download clean `linux-5.14.3` source
2. Apply the patches of the experiment (see the README inside the folder, the patches have to be applied with `-R`)
3. Build the kernel and boot into it
4. Compile and run the user programs inside the new kernel

## Folder structure

```
linux-kernel-experiments/
├── cfs-process-delay/
│   ├── kernel-patches/
│   ├── test/
│   └── screenshots/
└── producer-consumer-syscall/
    ├── kernel-patches/
    └── userspace/
```
