from datetime import datetime
import math
import random
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import HTMLResponse
from pydantic import BaseModel

app = FastAPI(
    title="Real-Time Disaster Alert & Prediction System API",
    version="1.0.0"
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

FILTER_ALPHA = 0.95
SEISMIC_GAIN = 2.5
SEISMIC_THRESHOLD = 0.20
SMOKE_THRESHOLD = 300.0

state = {
    "seismic_filtered": 0.018,
    "smoke_ppm": 115.0,
    "humidity_pct": 57.0,
}

class SensorPayload(BaseModel):
    node_id: str = "ESP32-NODE-01"
    ax: float
    ay: float
    az: float
    smoke_ppm: float
    humidity_pct: float

@app.get("/", response_class=HTMLResponse)
async def serve_dashboard():
    with open("index.html", "r", encoding="utf-8") as f:
        return HTMLResponse(content=f.read())

@app.get("/data")
async def get_data():
    raw_delta = abs(random.gauss(0.007, 0.004))
    state["seismic_filtered"] = round(
        (FILTER_ALPHA * state["seismic_filtered"]) +
        ((1.0 - FILTER_ALPHA) * raw_delta * SEISMIC_GAIN),
        4
    )
    s = state["seismic_filtered"]
    alert = "YES" if (s > SEISMIC_THRESHOLD or state["smoke_ppm"] > SMOKE_THRESHOLD) else "NO"
    return {
        "node_id": "ESP32-NODE-01",
        "timestamp": datetime.now().strftime("%H:%M:%S"),
        "seismic": s,
        "smoke_ppm": state["smoke_ppm"],
        "humidity_pct": state["humidity_pct"],
        "alert": alert
    }

@app.post("/ingest")
async def ingest_data(payload: SensorPayload):
    total_g = math.sqrt(payload.ax ** 2 + payload.ay ** 2 + payload.az ** 2)
    dynamic_vib = abs(total_g - 1.0)
    state["seismic_filtered"] = round(
        (FILTER_ALPHA * state["seismic_filtered"]) +
        ((1.0 - FILTER_ALPHA) * dynamic_vib * SEISMIC_GAIN),
        4
    )
    state["smoke_ppm"] = payload.smoke_ppm
    state["humidity_pct"] = payload.humidity_pct
    return await get_data()