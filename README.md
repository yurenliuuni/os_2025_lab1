# OS 2025 Lab1 Shared Memory & Message Passing IPC Performance Comparison

compares the performance of two fundamental Inter-Process Communication (IPC) mechanisms: System V Message Passing and POSIX Shared Memory. The communication between the sender and receiver processes is synchronized using POSIX Semaphores.

## 🚀 Features
- IPC Mechanisms: Implements two classic IPC techniques:

  1. System V Message Passing: Data is exchanged through a kernel-managed message queue (`msgget`, `msgsnd`, `msgrcv`).

  2. POSIX Shared Memory: Processes communicate by reading from and writing to a shared segment of memory (`shm_open`, `mmap`).

- Synchronization: Utilizes POSIX Semaphores (`sem_open`, `sem_wait`, `sem_post`) to ensure a correct, lock-step communication protocol between the sender and receiver.

- Performance Measurement: Uses `clock_gettime` to accurately measure the total wall-clock time for each communication session, allowing for a clear performance comparison.

