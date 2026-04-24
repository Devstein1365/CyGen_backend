import express from "express";
import cors from "cors";
import { createServer } from "http";

const app = express();
const PORT = 5000;
const MAX_READINGS = 1000; // In-memory buffer limit
const TELEMETRY_TIMEOUT_MS = 7000;

// Middleware
app.use(cors());
app.use(express.json());

// In-memory storage: bounded array of telemetry readings
let telemetryBuffer = [];

function normalizePayload(payload) {
  return {
    ...payload,
    // Arduino sketch may omit these; keep API compatible by defaulting.
    powerInput: Number.isFinite(payload.powerInput)
      ? payload.powerInput
      : payload.rpm * 2.5, // Mock conversion if missing
    powerOutput: Number.isFinite(payload.powerOutput)
      ? payload.powerOutput
      : payload.voltage
        ? payload.voltage * 2
        : 0,
    calories: Number.isFinite(payload.calories)
      ? payload.calories
      : Number.isFinite(payload.rpm)
        ? payload.rpm * 0.1
        : 0,
  };
}

function validatePayload(payload) {
  const requiredFields = [
    "battery",
    "chargingStatus",
    "rpm",
    "isPedaling",
    "etaToFull",
  ];

  return requiredFields.filter((f) => !(f in payload));
}

function buildLatestResponse(reading) {
  const ageMs = Date.now() - new Date(reading.serverTimestamp).getTime();
  const isConnected = ageMs <= TELEMETRY_TIMEOUT_MS;

  return {
    status: isConnected ? "ok" : "stale",
    data: reading,
    isConnected,
    connectionState: isConnected
      ? reading.isPedaling
        ? "active"
        : "idle"
      : "disconnected",
    lastSeenAt: reading.serverTimestamp,
    lastSeenAgoMs: ageMs,
  };
}

function buildWaitingResponse() {
  return {
    status: "waiting",
    data: null,
    message:
      "No telemetry data yet. ESP8266 has not sent readings. Waiting for first POST to /data.",
    isConnected: false,
  };
}

// Helper to add reading and maintain buffer size
function addReading(data) {
  const reading = {
    ...data,
    serverTimestamp: new Date().toISOString(),
    id: Date.now() + Math.random(),
  };
  telemetryBuffer.push(reading);
  if (telemetryBuffer.length > MAX_READINGS) {
    telemetryBuffer.shift(); // Remove oldest
  }
  return reading;
}

function getLatestReading() {
  if (telemetryBuffer.length === 0) return null;
  return telemetryBuffer[telemetryBuffer.length - 1];
}

// Health check endpoint
app.get("/health", (req, res) => {
  res.json({
    status: "ok",
    uptime: process.uptime(),
    timestamp: new Date().toISOString(),
  });
});

// Welcome endpoint
app.get("/", (req, res) => {
  res.send("<h1>CyGen Server Started</h1><p>Ready for ESP8266 telemetry.</p>");
});

// POST /data - ESP8266 telemetry ingestion
app.post("/data", (req, res) => {
  try {
    const payload = req.body;

    if (!payload || typeof payload !== "object") {
      return res.status(400).json({
        error: "Invalid payload",
        message: "Expected JSON object body",
      });
    }

    const normalizedPayload = normalizePayload(payload);

    // Validate required fields
    const missingFields = validatePayload(normalizedPayload);
    if (missingFields.length > 0) {
      return res.status(400).json({
        error: "Missing required fields",
        missing: missingFields,
      });
    }

    // Store reading
    const reading = addReading(normalizedPayload);

    res.status(200).json({
      status: "accepted",
      id: reading.id,
      message: "Telemetry received",
    });
  } catch (error) {
    console.error("Error processing telemetry:", error);
    res.status(500).json({
      error: "Internal server error",
      details: error.message,
    });
  }
});

// GET /api/measurements/latest - Fetch latest reading
app.get("/api/measurements/latest", (req, res) => {
  const latest = getLatestReading();
  if (!latest) {
    return res.status(200).json(buildWaitingResponse());
  }

  res.json(buildLatestResponse(latest));
});

// GET /api/measurements - Fetch telemetry history
app.get("/api/measurements", (req, res) => {
  const limit = Math.min(parseInt(req.query.limit) || 100, 500);
  const start = Math.max(0, telemetryBuffer.length - limit);
  const history = telemetryBuffer.slice(start);

  res.json({
    status: "ok",
    count: history.length,
    limit,
    data: history,
  });
});

// Catch 404
app.use((req, res) => {
  res.status(404).json({
    error: "Endpoint not found",
    path: req.path,
  });
});

const server = createServer(app);

// Start server on all interfaces (0.0.0.0) for LAN accessibility
server.listen(PORT, "0.0.0.0", () => {
  console.log(`CyGen server started at http://0.0.0.0:${PORT}`);
  console.log(
    `Ready to receive ESP8266 telemetry on http://<laptop-ip>:${PORT}/data`,
  );
  console.log(`Health check: http://localhost:${PORT}/health`);
});
