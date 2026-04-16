# Producer / Consumer via Custom Syscalls

A kernel-space FIFO queue exposed to userspace through three custom system calls, driving a classic producer/consumer workflow between two separate processes.

## Overview

Instead of using a filesystem pipe or a System V queue, this project builds the queue *inside the kernel* as a linked list in `sys.c`. Three syscalls manage it:

| Number | Syscall | Purpose |
|--------|---------|---------|
| 449 | `higgsQueue()` | Initialize the kernel queue (resets `start`, `tail`, and the busy-wait lock) |
| 450 | `writeCall(char *input)` | Enqueue 8 bytes copied from userspace |
| 451 | `readCall()` | Dequeue from the head and `printk` the bytes |

A shared integer `semaphore` (1 = free, 0 = held) guards enqueue/dequeue with a busy-wait spin.

## How It Works

```
 Producer userspace                Kernel space                  Consumer userspace
┌────────────────────┐       ┌────────────────────────┐       ┌────────────────────┐
│ syscall(449)       │──────▶│ higgsQueue: init list  │       │                    │
│ syscall(450, buf)  │──────▶│ writeCall: enqueue 8B  │       │                    │
│                    │       │  [Node→Node→Node]      │──────▶│ syscall(451)       │
│                    │       │ readCall: dequeue head │       │ (prints via printk)│
└────────────────────┘       └────────────────────────┘       └────────────────────┘
```

## Kernel Modifications

Target kernel: **Linux 5.14.3**. See [`kernel-patches/sys_c.diff`](./kernel-patches/sys_c.diff).

| File | Change |
|------|--------|
| `kernel/sys.c` | Adds headers, `struct Node` linked list, `start`/`tail`/`itr` pointers, busy-wait `semaphore`, and the three `SYSCALL_DEFINE*` handlers |
| `arch/x86/entry/syscalls/syscall_64.tbl` | Register syscalls 449, 450, 451 |

> **Note:** the original assignment README referred to the init syscall as `higgsFifo`; the actual symbol in the kernel is `higgsQueue`.

## Userspace

| File | Role |
|------|------|
| [`userspace/producer.c`](./userspace/producer.c) | Generates a random 19-digit number, calls `higgsQueue`, then `writeCall` to enqueue |
| [`userspace/consumer.c`](./userspace/consumer.c) | Calls `readCall` to dequeue and log via `dmesg` |

## Building and Running

```bash
# Apply patches to linux-5.14.3, rebuild, boot the patched kernel, then:
gcc userspace/producer.c -o producer
gcc userspace/consumer.c -o consumer

# Terminal 1
./producer

# Terminal 2
./consumer
dmesg | tail   # readCall output goes to kernel log
```

## Project Structure

```
producer-consumer-syscall/
├── kernel-patches/
│   └── sys_c.diff         # Full diff for kernel/sys.c
└── userspace/
    ├── producer.c
    └── consumer.c
```

## Caveats

- Busy-waiting in the kernel (spin on `semaphore != 1`) is a teaching shortcut, not production-quality locking — a real implementation should use `struct semaphore` / `mutex`
- `writeCall` allocates via `kmalloc` but `msg` is a static global pointer reused across calls; see `sys_c.diff` for exact semantics
- The producer also creates a filesystem FIFO (`mknod`) that the consumer reads — a second data path kept from earlier iterations of the assignment
