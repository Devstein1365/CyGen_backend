# CyGen Backend

This is the Node.js / Express backend API and WebSocket server for the CyGen application. It handles data processing and real-time communication with the frontend dashboard.

## Prerequisites

- [Node.js](https://nodejs.org/) (v16 or higher is recommended)
- [npm](https://www.npmjs.com/)

## Installation

Navigate to the `backend` directory and install the dependencies:

```bash
cd backend
npm install
```

## Running the Server

### Development Mode

To run the server in development mode with live reloading (using `nodemon`), run:

```bash
npm run dev
```

The server will typically start on your defined port (e.g., `http://localhost:3000` or `http://localhost:8080`) and provide the WebSocket connection.

## Dependencies

- **express:** Web framework for the API endpoints.
- **ws:** WebSocket library for real-time data streaming.
- **cors:** Middleware to enable Cross-Origin Resource Sharing.
- **nodemon:** Development utility to automatically restart the server upon file changes.
