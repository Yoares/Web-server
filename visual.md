Client            Kernel (epoll)           Webserv (Event Loop)        FileSystem
  |                     |                          |                        |
  | --- TCP SYN ------> |                          |                        |
  |                     | -- EPOLLIN (listen) -->  |                        |
  |                     |                          |                        |
  |                     |                          | accept()               |
  |                     |                          | set NONBLOCK           |
  |                     |                          | epoll_ctl(ADD, IN)     |
  |                     |                          |                        |
  |==================== PHASE 1 DONE =====================================|

  | --- GET /huge.mp4 ->|                          |                        |
  |                     | -- EPOLLIN (client) -->  |                        |
  |                     |                          | recv()                 |
  |                     |                          | parse request          |
  |                     |                          |                        |
  |                     |                          | stat(file)             |
  |                     |                          | open(file)             |
  |                     |                          |                        |
  |                     |                          | epoll_ctl(MOD, OUT)    |
  |                     |                          |                        |
  |==================== PHASE 2 DONE =====================================|

  |                     | -- EPOLLOUT -----------> |                        |
  |                     |                          | read(8KB) ------------>|
  |                     |                          | <------ data ----------|
  |                     |                          | send(8KB) ------------>|
  | <------ chunk ------|                          |                        |
  |                     |                          |                        |
  |                     |     🔁 YIELD (back to epoll_wait)                 |
  |                     |                          |                        |
  |                     | -- EPOLLOUT -----------> |                        |
  |                     |                          | read(8KB) ------------>|
  |                     |                          | send(8KB) ------------>|
  | <------ chunk ------|                          |                        |
  |                     |                          |                        |
  |==================== LOOP UNTIL EOF ================================|

  |                     |                          | read() -> EOF          |
  |                     |                          | close(file_fd)         |
  |                     |                          |                        |
  |                     |                          | epoll_ctl(MOD, IN)     |
  |                     |                          | OR close(client_fd)    |
  |                     |                          |                        |
  |==================== PHASE 4 DONE =====================================|