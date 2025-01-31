# Real-Time Chat Application

This is a real-time chat application built with a **C backend** and a **web frontend**.

## Features
- Real-time messaging using WebSockets.
- Simple and responsive web interface.
- Supports multiple clients.

## How to Run
1. **Backend**:
   - Compile the server: `gcc server.c -o server -lwebsockets`.
   - Run the server: `./server.exe`.

2. **Frontend**:
   - Serve the `index.html` file using an HTTP server: `python -m http.server 8000`.
   - Open `http://localhost:8000` in your browser.

## Technologies Used
- **Backend**: C, libwebsockets.
- **Frontend**: HTML, CSS, JavaScript.
