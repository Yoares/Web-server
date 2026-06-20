# *This project has been created as part of the 42 curriculum by [ykhoussi, obendaou].*

# Webserv

## Description

Webserv is a custom HTTP/1.1 web server developed in C++ as part of the 42 School curriculum.

The goal of this project is to understand how web servers work internally by implementing one from scratch. The server is capable of handling client connections, parsing HTTP requests, generating HTTP responses, serving static files, executing CGI scripts, managing file uploads, and supporting multiple virtual servers through a configuration file.

This project provides practical experience with:

* Network programming
* Socket management
* Event-driven architecture
* HTTP protocol fundamentals
* File handling
* Process management
* CGI execution
* Server configuration parsing

---

## Features

* HTTP/1.1 support
* Multiple virtual servers
* Non-blocking sockets
* Event multiplexing using `poll()`
* GET method support
* POST method support
* DELETE method support
* CGI execution
* Custom error pages
* File upload handling
* Directory listing (Autoindex)
* Session and cookie handling
* Request body parsing
* Multipart form-data parsing
* Configuration file parsing
* Connection timeout management

---

## Project Structure

```text
.
├── Makefile
├── README.md
├── config/
├── inc/
├── src/
├── www/
└── tests/
```

---

## Instructions

### Requirements

* Linux operating system
* C++98 compiler
* Make

### Compilation

```bash
make
```

This will generate the executable:

```bash
./webserv
```

### Running the Server

Run the server with the default configuration:

```bash
./webserv
```

Run the server with a custom configuration file:

```bash
./webserv config/default.conf
```

### Testing

Open your browser and access:

```text
http://localhost:8080
```

You can also test requests using:

```bash
curl http://localhost:8080
```

Example POST request:

```bash
curl -X POST \
     -F "file=@example.txt" \
     http://localhost:8080/upload
```

Example DELETE request:

```bash
curl -X DELETE \
     http://localhost:8080/file.txt
```

---

## Technical Choices

### Event Management

The server uses `poll()` to handle multiple simultaneous client connections without creating a thread per client.

### Configuration Parsing

The configuration file is inspired by Nginx syntax and allows:

* Multiple server blocks
* Route configuration
* Error page configuration
* CGI configuration
* Upload directory configuration

### CGI Support

CGI scripts are executed through `fork()` and `execve()` while redirecting input and output through pipes.

### File Uploads

Multipart form-data requests are parsed manually to extract uploaded files and save them to the configured upload directory.

---

## Resources

### HTTP Protocol

* RFC 7230 - Hypertext Transfer Protocol (HTTP/1.1): Message Syntax and Routing
* RFC 7231 - Hypertext Transfer Protocol (HTTP/1.1): Semantics and Content

### Network Programming

* Linux Manual Pages (`man socket`, `man bind`, `man listen`, `man accept`, `man poll`)
* The Linux Programming Interface — Michael Kerrisk

### CGI

* CGI/1.1 Specification (RFC 3875)

### C++

* C++98 Reference Documentation
* cppreference.com

### AI Usage

Artificial Intelligence tools were used as learning assistants during the development of this project.

AI was primarily used for:

* Understanding HTTP protocol concepts
* Clarifying RFC specifications
* Reviewing implementation ideas
* Explaining C++ language features
* Improving project documentation

---

## Authors

* ykhoussi
* obendaou

## License

This project was developed for educational purposes as part of the 42 School curriculum.
