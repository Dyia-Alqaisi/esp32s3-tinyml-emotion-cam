#include "WebPage.h"

namespace WebPage
{
    const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ALQAI EmotionCam</title>

    <style>
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }

        body {
            background-color: #f8fafc;
            color: #0f172a;
            font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            padding: 20px 16px;
            min-height: 100vh;
        }

        .container {
            max-width: 1100px;
            margin: 0 auto;
        }

        /* HEADER */
        .header {
            background: #ffffff;
            border: 1px solid #e2e8f0;
            border-radius: 14px;
            padding: 16px 20px;
            display: flex;
            align-items: center;
            justify-content: space-between;
            margin-bottom: 20px;
            box-shadow: 0 1px 3px rgba(0, 0, 0, 0.05);
        }

        .brand {
            display: flex;
            align-items: center;
            gap: 12px;
        }

        .brand-icon {
            width: 38px;
            height: 38px;
            background: #2563eb;
            color: #ffffff;
            border-radius: 10px;
            display: grid;
            place-items: center;
            font-weight: 700;
            font-size: 18px;
        }

        .brand-name {
            font-size: 18px;
            font-weight: 700;
            color: #0f172a;
        }

        .brand-sub {
            font-size: 12px;
            color: #64748b;
        }

        .status-group {
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .badge {
            display: inline-flex;
            align-items: center;
            gap: 6px;
            padding: 5px 12px;
            background: #f1f5f9;
            border: 1px solid #e2e8f0;
            border-radius: 999px;
            font-size: 12px;
            font-weight: 600;
            color: #475569;
        }

        .dot {
            width: 7px;
            height: 7px;
            border-radius: 50%;
            background: #16a34a;
        }

        .dot.error {
            background: #dc2626;
        }

        /* LAYOUT GRID */
        .grid {
            display: grid;
            grid-template-columns: 1.2fr 1fr;
            gap: 20px;
        }

        @media (max-width: 868px) {
            .grid {
                grid-template-columns: 1fr;
            }
        }

        /* CARDS */
        .card {
            background: #ffffff;
            border: 1px solid #e2e8f0;
            border-radius: 14px;
            padding: 20px;
            box-shadow: 0 1px 3px rgba(0, 0, 0, 0.05);
            margin-bottom: 20px;
        }

        .card:last-child {
            margin-bottom: 0;
        }

        .card-header {
            display: flex;
            align-items: center;
            justify-content: space-between;
            margin-bottom: 14px;
            padding-bottom: 10px;
            border-bottom: 1px solid #f1f5f9;
        }

        .card-title {
            font-size: 15px;
            font-weight: 700;
            color: #0f172a;
        }

        /* CAMERA FEED */
        .camera-box {
            position: relative;
            width: 100%;
            aspect-ratio: 4/3;
            background: #020617;
            border-radius: 10px;
            overflow: hidden;
            border: 1px solid #cbd5e1;
            display: flex;
            align-items: center;
            justify-content: center;
        }

        .camera-img {
            width: 100%;
            height: 100%;
            object-fit: cover;
            display: block;
        }

        /* Center Crop Target Overlay (400x400 on 640x480) */
        .crop-guide {
            position: absolute;
            top: 8.33%;
            left: 18.75%;
            width: 62.5%;
            height: 83.33%;
            border: 2px dashed #2563eb;
            border-radius: 8px;
            pointer-events: none;
            box-shadow: 0 0 0 9999px rgba(15, 23, 42, 0.35);
        }

        .crop-tag {
            position: absolute;
            top: 6px;
            right: 6px;
            background: #2563eb;
            color: #ffffff;
            font-size: 10px;
            font-weight: 700;
            padding: 2px 6px;
            border-radius: 4px;
        }

        /* BUTTONS */
        .btn-group {
            display: flex;
            gap: 10px;
            margin-top: 14px;
        }

        .btn {
            flex: 1;
            padding: 10px 14px;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 600;
            border: 1px solid transparent;
            cursor: pointer;
            transition: all 0.15s ease;
            font-family: inherit;
            text-align: center;
        }

        .btn-primary {
            background: #2563eb;
            color: #ffffff;
        }

        .btn-primary:hover:not(:disabled) {
            background: #1d4ed8;
        }

        .btn-secondary {
            background: #f8fafc;
            color: #334155;
            border-color: #cbd5e1;
        }

        .btn-secondary:hover:not(:disabled) {
            background: #f1f5f9;
            border-color: #94a3b8;
        }

        .btn:disabled {
            opacity: 0.6;
            cursor: not-allowed;
        }

        /* AI INFERENCE RESULT */
        .ai-result-card {
            background: #f8fafc;
            border: 1px solid #e2e8f0;
            border-radius: 10px;
            padding: 16px;
            text-align: center;
            margin-bottom: 16px;
        }

        .ai-emoji {
            font-size: 36px;
            margin-bottom: 4px;
        }

        .ai-pred-label {
            font-size: 20px;
            font-weight: 800;
            color: #0f172a;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }

        .ai-pred-sub {
            font-size: 12px;
            color: #64748b;
            margin-top: 2px;
        }

        /* PROBABILITY BARS */
        .prob-list {
            display: flex;
            flex-direction: column;
            gap: 10px;
            margin-bottom: 16px;
        }

        .prob-item {
            display: flex;
            flex-direction: column;
            gap: 4px;
        }

        .prob-meta {
            display: flex;
            justify-content: space-between;
            font-size: 12px;
            font-weight: 600;
        }

        .prob-lbl {
            color: #475569;
        }

        .prob-num {
            color: #0f172a;
            font-family: monospace;
            font-weight: 700;
        }

        .prob-track {
            height: 8px;
            background: #e2e8f0;
            border-radius: 999px;
            overflow: hidden;
        }

        .prob-fill {
            height: 100%;
            width: 0%;
            background: #2563eb;
            border-radius: 999px;
            transition: width 0.4s ease;
        }

        .prob-fill.active {
            background: #16a34a;
        }

        /* TIMING CHIPS */
        .timing-row {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 6px;
            margin-top: 12px;
        }

        .timing-chip {
            background: #f1f5f9;
            border-radius: 6px;
            padding: 6px 4px;
            text-align: center;
        }

        .timing-v {
            font-size: 12px;
            font-weight: 700;
            color: #2563eb;
        }

        .timing-l {
            font-size: 9px;
            color: #64748b;
            text-transform: uppercase;
            margin-top: 1px;
        }

        /* DATASET SELECTOR */
        .ds-grid {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 6px;
            margin-bottom: 12px;
        }

        .ds-pill {
            padding: 8px 4px;
            border-radius: 6px;
            background: #f1f5f9;
            border: 1px solid #cbd5e1;
            color: #475569;
            font-size: 11px;
            font-weight: 700;
            text-align: center;
            cursor: pointer;
        }

        .ds-pill.selected {
            background: #2563eb;
            color: #ffffff;
            border-color: #2563eb;
        }

        .ds-counts {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 6px;
            margin-top: 10px;
        }

        .ds-count-box {
            background: #f8fafc;
            border: 1px solid #e2e8f0;
            border-radius: 6px;
            padding: 6px;
            text-align: center;
        }

        .ds-count-num {
            font-size: 14px;
            font-weight: 700;
            color: #0f172a;
        }

        .ds-count-lbl {
            font-size: 9px;
            color: #64748b;
            text-transform: uppercase;
        }

        .sys-row {
            display: flex;
            justify-content: space-between;
            font-size: 12px;
            margin-bottom: 6px;
        }

        .sys-row:last-child {
            margin-bottom: 0;
        }

        .sys-val {
            font-weight: 700;
            color: #16a34a;
            font-family: monospace;
        }
    </style>
</head>
<body>
    <div class="container">
        <!-- HEADER -->
        <header class="header">
            <div class="brand">
                <div class="brand-icon">AI</div>
                <div>
                    <div class="brand-name">ALQAI EmotionCam</div>
                    <div class="brand-sub">ESP32-S3 + TFLite Micro Neural Vision</div>
                </div>
            </div>

            <div class="status-group">
                <div class="badge">
                    <span id="cameraDot" class="dot"></span>
                    <span>OV3660</span>
                </div>
                <div class="badge">
                    <span id="sdDot" class="dot"></span>
                    <span>SD MMC</span>
                </div>
            </div>
        </header>

        <!-- MAIN GRID -->
        <div class="grid">
            <!-- LEFT COLUMN: STREAM & SNAPSHOT CONTROLS -->
            <div>
                <div class="card">
                    <div class="card-header">
                        <span class="card-title">Live Stream</span>
                        <span id="cameraStatus" class="badge">Ready</span>
                    </div>

                    <div class="camera-box">
                        <img id="cameraImage" class="camera-img" src="" alt="Camera Feed">
                        <div class="crop-guide">
                            <span class="crop-tag">Crop 400x400</span>
                        </div>
                    </div>

                    <div class="btn-group">
                        <button id="pauseButton" class="btn btn-primary" type="button">Pause Stream</button>
                        <button id="captureButton" class="btn btn-secondary" type="button">Snapshot</button>
                        <button id="saveButton" class="btn btn-secondary" type="button">Save to SD</button>
                    </div>

                    <p id="actionMessage" style="font-size:12px; color:#64748b; margin-top:10px; text-align:center;">
                        Stream: http://192.168.4.1:81/stream
                    </p>
                </div>
            </div>

            <!-- RIGHT COLUMN: AI INFERENCE & DATASET -->
            <div>
                <!-- AI INFERENCE CARD -->
                <div class="card">
                    <div class="card-header">
                        <span class="card-title">AI Emotion Test</span>
                        <span id="aiStateVal" class="badge">READY</span>
                    </div>

                    <div class="ai-result-card">
                        <div id="aiEmoji" class="ai-emoji">😐</div>
                        <div id="aiPredictionVal" class="ai-pred-label">NEUTRAL</div>
                        <div id="aiSubText" class="ai-pred-sub">Press "Run AI Test" to analyze camera frame</div>
                    </div>

                    <div class="prob-list">
                        <div class="prob-item">
                            <div class="prob-meta">
                                <span class="prob-lbl">Neutral</span>
                                <span id="valNeutral" class="prob-num">0%</span>
                            </div>
                            <div class="prob-track">
                                <div id="barNeutral" class="prob-fill"></div>
                            </div>
                        </div>

                        <div class="prob-item">
                            <div class="prob-meta">
                                <span class="prob-lbl">Happy</span>
                                <span id="valHappy" class="prob-num">0%</span>
                            </div>
                            <div class="prob-track">
                                <div id="barHappy" class="prob-fill"></div>
                            </div>
                        </div>

                        <div class="prob-item">
                            <div class="prob-meta">
                                <span class="prob-lbl">Sad</span>
                                <span id="valSad" class="prob-num">0%</span>
                            </div>
                            <div class="prob-track">
                                <div id="barSad" class="prob-fill"></div>
                            </div>
                        </div>

                        <div class="prob-item">
                            <div class="prob-meta">
                                <span class="prob-lbl">Surprise</span>
                                <span id="valSurprise" class="prob-num">0%</span>
                            </div>
                            <div class="prob-track">
                                <div id="barSurprise" class="prob-fill"></div>
                            </div>
                        </div>
                    </div>

                    <div class="timing-row">
                        <div class="timing-chip">
                            <div id="tCap" class="timing-v">0ms</div>
                            <div class="timing-l">Cap</div>
                        </div>
                        <div class="timing-chip">
                            <div id="tDec" class="timing-v">0ms</div>
                            <div class="timing-l">Dec</div>
                        </div>
                        <div class="timing-chip">
                            <div id="tPrep" class="timing-v">0ms</div>
                            <div class="timing-l">Prep</div>
                        </div>
                        <div class="timing-chip">
                            <div id="tInf" class="timing-v">0ms</div>
                            <div class="timing-l">TFLite</div>
                        </div>
                    </div>

                    <button id="aiRunButton" class="btn btn-primary" type="button" style="width:100%; margin-top:14px;">
                        Run AI Test
                    </button>
                </div>

                <!-- DATASET COLLECTION CARD -->
                <div class="card">
                    <div class="card-header">
                        <span class="card-title">Dataset Collection</span>
                    </div>

                    <div class="ds-grid">
                        <div class="ds-pill selected" data-label="neutral">Neutral</div>
                        <div class="ds-pill" data-label="happy">Happy</div>
                        <div class="ds-pill" data-label="sad">Sad</div>
                        <div class="ds-pill" data-label="surprise">Surprise</div>
                    </div>

                    <div class="btn-group" style="margin-top:0;">
                        <button id="datasetCaptureButton" class="btn btn-secondary" type="button" style="flex:2;">
                            Capture Sample
                        </button>
                        <button id="datasetResetButton" class="btn btn-secondary" type="button" style="flex:1; color:#dc2626; border-color:#fca5a5;">
                            Reset
                        </button>
                    </div>

                    <div class="ds-counts">
                        <div class="ds-count-box">
                            <div id="cntNeutral" class="ds-count-num">0</div>
                            <div class="ds-count-lbl">Neutral</div>
                        </div>
                        <div class="ds-count-box">
                            <div id="cntHappy" class="ds-count-num">0</div>
                            <div class="ds-count-lbl">Happy</div>
                        </div>
                        <div class="ds-count-box">
                            <div id="cntSad" class="ds-count-num">0</div>
                            <div class="ds-count-lbl">Sad</div>
                        </div>
                        <div class="ds-count-box">
                            <div id="cntSurprise" class="ds-count-num">0</div>
                            <div class="ds-count-lbl">Surprise</div>
                        </div>
                    </div>
                </div>

                <!-- HARDWARE METERS CARD -->
                <div class="card">
                    <div class="sys-row">
                        <span style="color:#64748b;">Free Heap:</span>
                        <span id="heapStatus" class="sys-val">-- KB</span>
                    </div>
                    <div class="sys-row">
                        <span style="color:#64748b;">Free PSRAM:</span>
                        <span id="psramStatus" class="sys-val">-- MB</span>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <!-- JAVASCRIPT -->
    <script>
        var cameraImage = document.getElementById("cameraImage");
        var pauseButton = document.getElementById("pauseButton");
        var captureButton = document.getElementById("captureButton");
        var saveButton = document.getElementById("saveButton");
        var actionMessage = document.getElementById("actionMessage");

        var aiRunButton = document.getElementById("aiRunButton");
        var aiStateVal = document.getElementById("aiStateVal");
        var aiEmoji = document.getElementById("aiEmoji");
        var aiPredictionVal = document.getElementById("aiPredictionVal");
        var aiSubText = document.getElementById("aiSubText");

        var valNeutral = document.getElementById("valNeutral");
        var valHappy = document.getElementById("valHappy");
        var valSad = document.getElementById("valSad");
        var valSurprise = document.getElementById("valSurprise");

        var barNeutral = document.getElementById("barNeutral");
        var barHappy = document.getElementById("barHappy");
        var barSad = document.getElementById("barSad");
        var barSurprise = document.getElementById("barSurprise");

        var tCap = document.getElementById("tCap");
        var tDec = document.getElementById("tDec");
        var tPrep = document.getElementById("tPrep");
        var tInf = document.getElementById("tInf");

        var selectedLabel = "neutral";
        var previewRunning = false;
        var aiTimer = null;

        var EMOJI_MAP = {
            "neutral": "😐",
            "happy": "😊",
            "sad": "😢",
            "surprise": "😲"
        };

        function getStreamUrl() {
            var host = window.location.hostname || "192.168.4.1";
            return "http://" + host + ":81/stream";
        }

        function startLiveStream() {
            cameraImage.src = getStreamUrl();
            previewRunning = true;
            pauseButton.textContent = "Pause Stream";
            actionMessage.textContent = "Stream active";
            fetch("/ui-event?action=resume").catch(function(){});
        }

        function stopLiveStream(msg) {
            previewRunning = false;
            cameraImage.src = "";
            pauseButton.textContent = "Start Stream";
            actionMessage.textContent = msg || "Stream paused";
            fetch("/ui-event?action=pause").catch(function(){});
        }

        pauseButton.addEventListener("click", function() {
            if (previewRunning) {
                stopLiveStream("Stream paused");
            } else {
                startLiveStream();
            }
        });

        captureButton.addEventListener("click", function() {
            stopLiveStream("Capturing snapshot...");
            cameraImage.src = "/capture?t=" + Date.now();
        });

        saveButton.addEventListener("click", function() {
            actionMessage.textContent = "Saving image to SD card...";
            fetch("/save", { cache: "no-store" })
                .then(function(r) { return r.json(); })
                .then(function(d) {
                    if (d.file) {
                        actionMessage.textContent = "Saved: " + d.file;
                    } else {
                        actionMessage.textContent = "Save failed";
                    }
                })
                .catch(function() {
                    actionMessage.textContent = "Save request error";
                });
        });

        function softmax(arr) {
            var max = Math.max.apply(null, arr);
            var exps = arr.map(function(x) { return Math.exp(x - max); });
            var sum = exps.reduce(function(a, b) { return a + b; }, 0);
            return exps.map(function(e) { return e / sum; });
        }

        function pollAiStatus() {
            fetch("/ai/status", { cache: "no-store" })
                .then(function(r) { return r.json(); })
                .then(function(st) {
                    aiStateVal.textContent = st.state ? st.state.toUpperCase() : "READY";

                    if (st.prediction && st.prediction !== "none") {
                        var p = st.prediction.toLowerCase();
                        aiPredictionVal.textContent = p.toUpperCase();
                        aiEmoji.textContent = EMOJI_MAP[p] || "😐";
                        aiSubText.textContent = "Latency: " + (st.totalMs / 1000).toFixed(2) + "s";
                    }

                    if (st.logits && st.logits.length === 4) {
                        var probs = softmax(st.logits);
                        var pN = Math.round(probs[0] * 100);
                        var pH = Math.round(probs[1] * 100);
                        var pS = Math.round(probs[2] * 100);
                        var pSu = Math.round(probs[3] * 100);

                        valNeutral.textContent = pN + "%";
                        valHappy.textContent = pH + "%";
                        valSad.textContent = pS + "%";
                        valSurprise.textContent = pSu + "%";

                        barNeutral.style.width = pN + "%";
                        barHappy.style.width = pH + "%";
                        barSad.style.width = pS + "%";
                        barSurprise.style.width = pSu + "%";

                        [barNeutral, barHappy, barSad, barSurprise].forEach(function(b) { b.classList.remove("active"); });
                        if (st.predictionIndex === 0) barNeutral.classList.add("active");
                        if (st.predictionIndex === 1) barHappy.classList.add("active");
                        if (st.predictionIndex === 2) barSad.classList.add("active");
                        if (st.predictionIndex === 3) barSurprise.classList.add("active");
                    }

                    if (st.captureMs) tCap.textContent = st.captureMs + "ms";
                    if (st.decodeMs) tDec.textContent = st.decodeMs + "ms";
                    if (st.preprocessMs) tPrep.textContent = st.preprocessMs + "ms";
                    if (st.inferenceMs) tInf.textContent = st.inferenceMs + "ms";

                    if (st.busy) {
                        aiRunButton.disabled = true;
                        if (!aiTimer) {
                            aiTimer = setInterval(pollAiStatus, 500);
                        }
                    } else {
                        aiRunButton.disabled = false;
                        if (aiTimer) {
                            clearInterval(aiTimer);
                            aiTimer = null;
                        }
                    }
                })
                .catch(function() {});
        }

        aiRunButton.addEventListener("click", function() {
            aiRunButton.disabled = true;
            aiStateVal.textContent = "QUEUED...";
            fetch("/ai/run", { method: "POST", cache: "no-store" })
                .then(function(r) { return r.json(); })
                .then(function(j) {
                    if (j.accepted) {
                        pollAiStatus();
                    } else {
                        aiStateVal.textContent = j.reason || "REJECTED";
                        aiRunButton.disabled = false;
                    }
                })
                .catch(function() {
                    aiStateVal.textContent = "ERROR";
                    aiRunButton.disabled = false;
                });
        });

        // DATASET PILL SELECTOR
        var pills = document.querySelectorAll(".ds-pill");
        pills.forEach(function(pill) {
            pill.addEventListener("click", function() {
                pills.forEach(function(p) { p.classList.remove("selected"); });
                pill.classList.add("selected");
                selectedLabel = pill.getAttribute("data-label");
            });
        });

        document.getElementById("datasetCaptureButton").addEventListener("click", function() {
            actionMessage.textContent = "Capturing " + selectedLabel + " dataset sample...";
            fetch("/dataset/capture?label=" + encodeURIComponent(selectedLabel), { method: "POST", cache: "no-store" })
                .then(function(r) { return r.json(); })
                .then(function(res) {
                    if (res.success) {
                        actionMessage.textContent = selectedLabel + " sample saved!";
                        updateDatasetStatus();
                    } else {
                        actionMessage.textContent = "Dataset capture failed";
                    }
                })
                .catch(function() {
                    actionMessage.textContent = "Dataset capture error";
                });
        });

        document.getElementById("datasetResetButton").addEventListener("click", function() {
            if (!confirm("Are you sure you want to reset all dataset counts and clear images?")) return;
            actionMessage.textContent = "Resetting dataset...";
            fetch("/dataset/reset", { method: "POST", cache: "no-store" })
                .then(function(r) { return r.json(); })
                .then(function(res) {
                    if (res.success) {
                        actionMessage.textContent = "Dataset reset successfully!";
                        updateDatasetStatus();
                    } else {
                        actionMessage.textContent = "Reset failed";
                    }
                })
                .catch(function() {
                    actionMessage.textContent = "Reset error";
                });
        });

        function updateDatasetStatus() {
            fetch("/dataset/status", { cache: "no-store" })
                .then(function(r) { return r.json(); })
                .then(function(d) {
                    document.getElementById("cntNeutral").textContent = d.neutral || 0;
                    document.getElementById("cntHappy").textContent = d.happy || 0;
                    document.getElementById("cntSad").textContent = d.sad || 0;
                    document.getElementById("cntSurprise").textContent = d.surprise || 0;
                })
                .catch(function() {});
        }

        function updateStatus() {
            fetch("/status", { cache: "no-store" })
                .then(function(r) { return r.json(); })
                .then(function(st) {
                    document.getElementById("cameraDot").classList.toggle("error", !st.camera);
                    document.getElementById("sdDot").classList.toggle("error", !st.sd);
                    document.getElementById("heapStatus").textContent = Math.round(st.heap / 1024) + " KB";
                    document.getElementById("psramStatus").textContent = (st.psram / 1024 / 1024).toFixed(2) + " MB";
                    document.getElementById("cameraStatus").textContent = st.stream ? "Streaming" : "Ready";
                })
                .catch(function() {});
        }

        window.addEventListener("load", function() {
            startLiveStream();
            updateStatus();
            updateDatasetStatus();
            pollAiStatus();
        });

        setInterval(updateStatus, 3000);
        setInterval(updateDatasetStatus, 6000);
    </script>
</body>
</html>
)HTML";
}
