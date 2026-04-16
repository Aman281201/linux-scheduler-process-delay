# Linux Kernel Experiments

A hands-on dive into the **Linux 5.14.3** kernel internals — centered on hacking the **Completely Fair Scheduler (CFS)** to throttle a process on demand, with a second experiment in kernel-space IPC via custom syscalls.

---

## The Main Event — Hacking the CFS Scheduler

**Sub-project:** [`cfs-process-delay/`](./cfs-process-delay)

### Background: How CFS Works

Linux's default scheduler since 2007, the Completely Fair Scheduler replaced the older O(1) scheduler. Its goal is to give every runnable task a **proportional share** of CPU time relative to its weight (derived from its nice value). The central abstraction is **virtual runtime** — `vruntime`, measured in nanoseconds.

The core mechanics:

- Every task carries a `vruntime` counter on its `struct sched_entity`
- All runnable tasks on a CPU live in a **red-black tree** keyed on `vruntime`
- The scheduler always picks the task at the **leftmost node** — the one that has consumed the least virtual CPU time so far
- While the task runs, its `vruntime` grows; once it exceeds its neighbors', it drifts rightward in the tree and someone else is selected
- Higher-priority (lower nice) tasks have `vruntime` advance *more slowly*, so they stay leftward and get selected more often — fairness with priority baked in

```
         [  root  ]                   leftmost = next to run
        /          \
  [lower]          [higher]
   /    \           /    \
[lowest] ...  ...           [highest vruntime → runs last]
   ^
   └─ scheduler picks this one
```

Every scheduler tick funnels through a single function — `update_curr()` in `kernel/sched/fair.c` — which advances the running task's `vruntime`. That single chokepoint is the hook.

### The Hack

Add one field to `sched_entity`, inject it into `update_curr`, and expose a syscall to set it per-PID. Result: a per-process throttle, implementable in ~5 lines of patches.

```c
/* kernel/sched/fair.c — update_curr */
curr->vruntime += calc_delta_fair(delta_exec, curr) + curr->delayAdd;
/*                                                   ^^^^^^^^^^^^^^^
 *                                                   injected delta
 *                                                   set by syscall
 */
```

Because CFS's entire fairness model is built around `vruntime` comparisons, anyone who can write to a task's `vruntime` can make the scheduler *think* it has already been served — pushing it rightward in the red-black tree and starving it of CPU. It's a soft, userspace-controllable analog of nice.

### Userspace Interface

```c
long higgsCall_delay(int pid, int delay_ms);
```

Returns `0` on success, `-ESRCH` if `pid` doesn't exist. The `delay_ms` argument is converted to nanoseconds and stored as `delayAdd` on the target task's `sched_entity`. From that point on, every tick adds `delay_ms × 10⁶` ns of *virtual* runtime to the process, without the task having actually run that long.

### What's Patched

| File | Change |
|------|--------|
| `include/linux/sched.h` | Add `u64 delayAdd` to `struct sched_entity` |
| `kernel/sched/core.c` | Initialize `delayAdd = 0` in `__sched_fork` |
| `kernel/sched/fair.c` | Fold `delayAdd` into `vruntime` inside `update_curr` |
| `kernel/sys.c` | Implement `SYSCALL_DEFINE2(higgsCall_delay, ...)` — resolves PID and writes the field |
| `arch/x86/entry/syscalls/syscall_64.tbl` | Register syscall number 448 |

### Why It's Interesting

A minimal case study in:

- **Scheduler design** — how CFS's data structures (the red-black tree) and invariants (`vruntime` ordering) compose
- **Kernel extensibility** — how adding a single syscall grants userspace controlled access to a per-task kernel field
- **Why CFS was designed this way** — because the update path funnels through one function, the whole scheduler can be reshaped in a few lines

Full write-up, patches, test harness, and CFS screenshots in [`cfs-process-delay/`](./cfs-process-delay).

---

## Bonus Experiment — Kernel-Space Producer/Consumer

**Sub-project:** [`producer-consumer-syscall/`](./producer-consumer-syscall)

A second experiment on the same kernel: instead of routing producer/consumer IPC through a filesystem FIFO or a System V message queue, the **queue itself lives inside the kernel** as a linked list in `sys.c`. Three custom syscalls manage it, and two separate userspace programs drive them.

| Number | Syscall | Role |
|--------|---------|------|
| 449 | `higgsQueue()` | Initialize the kernel queue |
| 450 | `writeCall(buf)` | Enqueue 8 bytes from userspace |
| 451 | `readCall()` | Dequeue and print via `printk` |

See [`producer-consumer-syscall/`](./producer-consumer-syscall) for the full design and patch.

---

## Common Setup

Both experiments target the same kernel version and follow the same workflow:

1. Download a clean `linux-5.14.3` source tree
2. Apply the patches from the relevant sub-project's `kernel-patches/` directory
3. Register new syscall numbers in `arch/x86/entry/syscalls/syscall_64.tbl`
4. Rebuild the kernel and boot into the patched image
5. Compile and run the userspace programs inside that kernel

## Syscall Map

| Number | Name | Project |
|--------|------|---------|
| 448 | `higgsCall_delay` | cfs-process-delay |
| 449 | `higgsQueue` | producer-consumer-syscall |
| 450 | `writeCall` | producer-consumer-syscall |
| 451 | `readCall` | producer-consumer-syscall |

## Repository Layout

```
linux-kernel-experiments/
├── cfs-process-delay/              # CFS scheduler modification (main project)
│   ├── kernel-patches/
│   ├── test/
│   ├── screenshots/
│   └── binaries/
└── producer-consumer-syscall/      # Kernel-space FIFO queue (bonus)
    ├── kernel-patches/
    └── userspace/
```
