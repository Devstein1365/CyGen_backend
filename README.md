# CyGen Backend

This folder contains two parts of the CyGen backend system:

- Node.js API server (`index.js`) that receives and serves telemetry.
- Microcontroller firmware sketch (`CyGen.ino`) that reads sensor values and POSTs them to the backend.

## Architecture

1. Device runs `CyGen.ino` and sends JSON telemetry to `POST /data` every 2 seconds.
2. Node backend stores recent readings in memory.
3. Frontend polls `GET /api/measurements/latest` to render live dashboard values.

## Prerequisites

### Backend (Node.js)

- [Node.js](https://nodejs.org/) (v16+ recommended)
- [npm](https://www.npmjs.com/)

### Firmware (`CyGen.ino`)

- Arduino IDE (or PlatformIO)
- ESP8266 board package installed in Arduino IDE
- Libraries used by sketch:
  - `ESP8266WiFi`
  - `ESP8266HTTPClient`
  - `Wire`
  - `LiquidCrystal_I2C`

Note: The current sketch uses ESP8266-specific headers and pin names (`D5`), so it targets ESP8266-class boards (for example NodeMCU). If you are using Arduino Uno, adapt board libraries and networking hardware/code accordingly.

## Setup

### 1. Install backend dependencies

```bash
cd backend
npm install
```

### 2. Configure firmware network settings

Open `CyGen.ino` and update:

- `ssid`
- `password`
- `serverUrl` (set to your computer IP and backend port, for example `http://192.168.0.145:5000/data`)

Important: the board and laptop must be on the same network.

## Run

### Start backend server

```bash
cd backend
npm run dev
```

Server defaults to port `5000` and binds to `0.0.0.0` for LAN access.

### Flash firmware

1. Connect your ESP8266 board.
2. Select the correct board and port in Arduino IDE.
3. Upload `CyGen.ino`.
4. Open Serial Monitor at `115200` baud to verify connection and POST responses.

## API Endpoints

- `GET /health`
  - Service health check.
- `POST /data`
  - Ingest telemetry from device.
  - Required fields: `battery`, `chargingStatus`, `rpm`, `isPedaling`, `etaToFull`.
- `GET /api/measurements/latest`
  - Returns latest reading or `status: "waiting"` when no data yet.
- `GET /api/measurements?limit=100`
  - Returns recent telemetry history.

## Example Payload

```json
{
  "battery": 78,
  "voltage": 12.12,
  "rpm": 64,
  "chargingStatus": "charging",
  "isPedaling": true,
  "etaToFull": 11
}
```

## Demo Workflow (No Device)

For portfolio demos without hardware:

1. Run backend (`npm run dev`).
2. Run frontend (`npm run dev` in `frontend`).
3. Frontend will show full UI and waiting/demo states until real telemetry arrives.

## Dependencies

- `express`: API framework.
- `cors`: Cross-origin requests for frontend.
- `ws`: Installed for realtime support needs.
- `nodemon`: Auto-restart during development.
