# CFS Process Delay

A Linux kernel modification that introduces a custom system call to artificially slow down a target process by manipulating its virtual runtime (`vruntime`) inside the Completely Fair Scheduler.

## Overview

The Linux CFS scheduler keeps runnable tasks in a red-black tree ordered by `vruntime`. The task with the lowest `vruntime` sits at the leftmost node and is picked to run next. By adding a configurable delta to a target process's `vruntime` on every scheduler update, we push it rightward in the tree — giving it a smaller share of CPU time.

This project adds:

- A new field `delayAdd` on `struct sched_entity`
- A system call `higgsCall_delay(pid, delay_ms)` that sets `delayAdd` on any process by PID
- A modified `update_curr()` in `fair.c` that folds `delayAdd` into `vruntime` each tick

## How It Works

| Step | What Happens |
|------|--------------|
| 1 | Userspace calls `syscall(448, pid, delay_ms)` |
| 2 | Kernel looks up the `task_struct` for `pid` |
| 3 | `se.delayAdd` is set to `delay_ms * 10^6` (ns) |
| 4 | On every scheduler update, `vruntime += delayAdd` |
| 5 | Task drifts rightward in the CFS red-black tree → lower effective priority |

## Kernel Modifications

Target kernel: **Linux 5.14.3**. Unified diffs live in [`kernel-patches/`](./kernel-patches).

| File | Patch | Change |
|------|-------|--------|
| `include/linux/sched.h` | `sched_h.patch` | Add `u64 delayAdd` to `struct sched_entity` |
| `kernel/sched/core.c` | `core_c.patch` | Initialize `delayAdd = 0` in `__sched_fork` |
| `kernel/sched/fair.c` | `fair_c.patch` | Add `delayAdd` to `vruntime` in `update_curr` |
| `kernel/sys.c` | `sys_c.patch` | Implement `SYSCALL_DEFINE2(higgsCall_delay, ...)` |
| `arch/x86/entry/syscalls/syscall_64.tbl` | `syscall_table.patch` | Register syscall number 448 |

## System Call

```c
long higgsCall_delay(int pid, int delay_ms);
```

Returns `0` on success, `-ESRCH` if `pid` does not exist. `delay_ms` is converted to nanoseconds inside the kernel.

## Building and Testing

Apply the patches to a clean `linux-5.14.3` tree, rebuild, and boot into the patched kernel. Then compile and run the userspace test:

```bash
cd test
gcc -fopenmp test.c -o test
./test
```

The test forks 10 baseline processes with no delay, times them, then forks 10 more with a 1000 ms delay and compares completion times.

## Project Structure

```
cfs-process-delay/
├── kernel-patches/   # Unified diffs for the kernel tree
├── test/             # Userspace test harness
├── screenshots/      # Scheduler diagrams and sample output
└── binaries/         # Pre-compiled test binary
```

## Screenshots

See [`screenshots/`](./screenshots) for CFS internals and sample test results.
