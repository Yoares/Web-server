# Webserv 🌐

A custom, high-performance HTTP web server written from scratch in C++98. Inspired by NGINX, this project demonstrates a deep understanding of low-level network programming, kernel-level socket management, and non-blocking I/O multiplexing. 

This repository serves not only as a functional server but as a comprehensive study of the OSI model, system calls, and strict memory management required to build robust infrastructure.

---

## 🏗️ Architecture & Core Modules

This server is designed around a single-threaded, event-driven architecture utilizing I/O multiplexing (`poll()`/`epoll()`) to handle thousands of concurrent connections without blocking. It strictly adheres to the C++98 standard.

### 1. The Core Engine
* **Webserv (Master Orchestrator):** The central event loop that monitors all file descriptors for read/write readiness.
* **Connection (State Machine):** Manages the lifecycle of each client, maintaining independent read and write buffers to prevent data fragmentation issues.
* **HttpRequest (Parser):** Iteratively consumes raw bytes, parsing headers and body data chunk-by-chunk.
* **HttpResponse (Router):** Evaluates the fully parsed request against the configuration tree to serve static files, generate error pages, or trigger CGI scripts.

### 2. Configuration Parser
The server is dynamically driven by an NGINX-style configuration file (`.conf`). The parsing engine prevents segmentation faults and undefined behavior through a strict three-layer pipeline:
1.  **Tokenizer (Lexical Analysis):** Strips comments and whitespace, generating a clean array of syntax tokens.
2.  **Recursive Descent Parser (Syntax Analysis):** Consumes tokens to build a hierarchical Abstract Syntax Tree (AST), linking Server blocks to their respective Location blocks.
3.  **Semantic Validator:** Verifies the logical soundness of the AST before boot (e.g., ensuring ports are valid, root directories exist, and there are no conflicting directives).

### 3. Non-Blocking CGI Execution
Handling dynamic content (PHP/Python) without stalling the main event loop is achieved through a custom `CgiHandler`.
* Uses `fork()` to create a child process and `execve()` to run the target script.
* Communication is established via `pipe()`. 
* **Crucially:** The read-end of the pipe is injected directly into the main `poll()`/`epoll()` loop. The server treats the executing script exactly like another client socket, reading its output only when the kernel signals it is ready.

---

## ⚙️ Low-Level Networking Fundamentals

### The Client-Server Lifecycle
The server establishes communication channels by manipulating system resources through File Descriptors (fds).
1.  **`socket()`:** Allocates a raw handle for network communication using `AF_INET` (IPv4) and `SOCK_STREAM` (TCP) for reliable, connection-oriented data transfer.
2.  **`bind()`:** Claims a specific IP and Port within the kernel. Utilizing `INADDR_ANY` (`serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);`) ensures the server listens across all available network interfaces (Localhost, Wi-Fi, Ethernet), maximizing portability.
3.  **`listen()`:** Toggles the socket's internal kernel state from active to passive (`TCP_LISTEN`), instructing the OS to begin queuing incoming SYN packets into the SYN and Accept queues.
4.  **`accept()`:** Extracts a fully completed TCP handshake from the kernel's Accept Queue, generating a brand-new file descriptor specifically for that client while the original socket continues listening.

### Memory & Struct Casting
Network system calls require generic pointer types to handle various address formats (IPv4 vs. IPv6). The architecture utilizes C-style casting to safely transition between `sockaddr_in` (which we populate) and `sockaddr` (which the kernel requires) to achieve polymorphism in C++98.

---

## 🧠 Defense Preparation: "Under the Hood" Mechanics

This section documents the underlying kernel logic to ensure a deep, conceptual mastery of the systems at play.

* **The Backlog Myth:** The integer passed to `listen(server_fd, backlog)` does not represent the maximum number of concurrent users. It strictly defines the size of the kernel's Accept Queue—the "waiting room" for connections that have finished the TCP handshake but haven't yet been `accept()`ed by the application.
* **Kernel Strictness:** The OS kernel acts as a ruthless gatekeeper. It enforces double checksums on incoming IP/TCP headers and validates protocol matching before data ever reaches the application layer.
* **`TIME_WAIT` & `SO_REUSEADDR`:** Handled the "Address Already in Use" error during iterative development by manipulating socket options, understanding how the kernel intentionally holds ports open temporarily to catch delayed, wandering packets from closed connections.
* **Privileged Ports:** Acknowledged the security constraints enforced by the kernel, requiring root privileges to bind to well-known ports (1–1023).

---

## 🚀 Getting Started

*(Add your compilation and execution instructions here)*

```bash
# Example
make
./webserv config/default.conf