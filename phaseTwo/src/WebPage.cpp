#include "WebPage.h"

namespace WebPage
{
    const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta
        name="viewport"
        content="width=device-width, initial-scale=1.0"
    >
    <title>ALQAI EmotionCam</title>

    <style>
        * {
            box-sizing: border-box;
        }

        :root {
            --background: #f5f7fa;
            --surface: #ffffff;
            --navy: #10243e;
            --blue: #2474e5;
            --yellow: #f2c94c;
            --green: #239b68;
            --red: #cc3d3d;
            --text-secondary: #667085;
            --border: #dce2ea;
            --soft-background: #f8fafc;
        }

        body {
            margin: 0;
            padding: 24px;
            background: var(--background);
            color: var(--navy);
            font-family: Arial, Helvetica, sans-serif;
        }

        .app {
            width: 100%;
            max-width: 1050px;
            margin: 0 auto;
            overflow: hidden;
            background: var(--surface);
            border: 1px solid var(--border);
            border-radius: 22px;
        }

        .header {
            display: flex;
            align-items: center;
            justify-content: space-between;
            gap: 16px;
            padding: 18px 24px;
            border-bottom: 1px solid var(--border);
        }

        .brand {
            display: flex;
            align-items: center;
            gap: 12px;
        }

        .brand-mark {
            display: grid;
            width: 38px;
            height: 38px;
            place-items: center;
            border-radius: 10px;
            background: var(--yellow);
            color: var(--navy);
            font-size: 19px;
            font-weight: bold;
        }

        .brand-row {
            display: flex;
            align-items: center;
            gap: 9px;
        }

        .brand-name {
            margin: 0;
            font-size: 18px;
            font-weight: bold;
            letter-spacing: 0.7px;
        }

        .device-badge {
            padding: 4px 8px;
            border-radius: 999px;
            background: #eef4fd;
            color: var(--blue);
            font-size: 11px;
            font-weight: bold;
        }

        .product-name {
            margin: 3px 0 0;
            color: var(--text-secondary);
            font-size: 14px;
        }

        .online-status {
            display: flex;
            align-items: center;
            gap: 8px;
            color: var(--text-secondary);
            font-size: 14px;
        }

        .status-dot {
            display: inline-block;
            width: 8px;
            height: 8px;
            flex-shrink: 0;
            border-radius: 50%;
            background: var(--green);
        }

        .status-dot.error {
            background: var(--red);
        }

        .main-layout {
            display: grid;
            grid-template-columns:
                minmax(0, 1.65fr)
                minmax(280px, 0.85fr);
        }

        .camera-section {
            padding: 24px;
            background: var(--background);
        }

        .camera-panel {
            overflow: hidden;
            background: var(--surface);
            border: 1px solid var(--border);
            border-radius: 17px;
        }

        .camera-frame {
            position: relative;
            display: flex;
            width: 100%;
            aspect-ratio: 4 / 3;
            align-items: center;
            justify-content: center;
            overflow: hidden;
            background: #101b2d;
        }

        #cameraImage {
            display: block;
            width: 100%;
            height: 100%;
            object-fit: contain;
        }

        .live-badge {
            position: absolute;
            top: 15px;
            left: 15px;
            padding: 7px 11px;
            border-radius: 999px;
            background: rgba(0, 0, 0, 0.55);
            color: #ffffff;
            font-size: 11px;
            font-weight: bold;
            letter-spacing: 0.5px;
        }

        .live-dot {
            display: inline-block;
            width: 7px;
            height: 7px;
            margin-right: 6px;
            border-radius: 50%;
            background: #ff5656;
        }

        .face-guide {
            position: absolute;
            top: 50%;
            left: 50%;
            width: 37%;
            aspect-ratio: 3 / 4;
            pointer-events: none;
            border: 2px solid rgba(255, 255, 255, 0.76);
            border-radius: 44% 44% 40% 40%;
            transform: translate(-50%, -50%);
        }

        .guide-message {
            position: absolute;
            bottom: 15px;
            left: 50%;
            padding: 8px 14px;
            border-radius: 999px;
            background: rgba(0, 0, 0, 0.58);
            color: #ffffff;
            font-size: 13px;
            white-space: nowrap;
            transform: translateX(-50%);
        }

        .control-area {
            display: flex;
            align-items: center;
            justify-content: space-between;
            gap: 14px;
            padding: 15px;
            border-top: 1px solid var(--border);
        }

        .buttons {
            display: flex;
            flex-wrap: wrap;
            gap: 9px;
        }

        button {
            min-height: 42px;
            padding: 10px 16px;
            border-radius: 9px;
            font-size: 14px;
            font-weight: bold;
            cursor: pointer;
        }

        button:disabled {
            cursor: not-allowed;
            opacity: 0.55;
        }

        .pause-button {
            border: 1px solid #ccd4df;
            background: #ffffff;
            color: var(--navy);
        }

        .capture-button {
            border: 1px solid var(--navy);
            background: var(--navy);
            color: #ffffff;
        }

        .save-button {
            border: 1px solid var(--blue);
            background: var(--blue);
            color: #ffffff;
        }

        #actionMessage {
            margin: 0;
            color: var(--text-secondary);
            font-size: 13px;
            text-align: right;
        }

        .side-panel {
            padding: 24px;
            border-left: 1px solid var(--border);
            background: var(--surface);
        }

        .section-title {
            margin: 0 0 12px;
            color: var(--text-secondary);
            font-size: 13px;
            font-weight: bold;
        }

        .status-list {
            overflow: hidden;
            border: 1px solid var(--border);
            border-radius: 12px;
        }

        .status-row {
            display: flex;
            align-items: center;
            justify-content: space-between;
            gap: 10px;
            padding: 13px 14px;
            border-bottom: 1px solid #edf0f4;
        }

        .status-row:last-child {
            border-bottom: none;
        }

        .status-label {
            color: var(--text-secondary);
            font-size: 13px;
        }

        .status-value {
            display: flex;
            align-items: center;
            gap: 7px;
            color: var(--navy);
            font-size: 13px;
            font-weight: bold;
        }

        .capture-information {
            margin-top: 24px;
        }

        .latest-capture {
            padding: 15px;
            border: 1px solid var(--border);
            border-radius: 12px;
            background: var(--soft-background);
        }

        .latest-file {
            margin: 0;
            color: var(--navy);
            font-size: 14px;
            font-weight: bold;
            overflow-wrap: anywhere;
        }

        .latest-details {
            margin: 6px 0 0;
            color: var(--text-secondary);
            font-size: 13px;
            line-height: 1.5;
        }

        .dataset-section {
            margin-top: 24px;
        }

        .dataset-card {
            padding: 15px;
            border: 1px solid var(--border);
            border-radius: 12px;
            background: var(--soft-background);
        }

        .dataset-label {
            display: block;
            margin-bottom: 7px;
            color: var(--text-secondary);
            font-size: 12px;
            font-weight: bold;
        }

        .dataset-select {
            width: 100%;
            min-height: 42px;
            padding: 9px 11px;
            border: 1px solid #ccd4df;
            border-radius: 9px;
            background: #ffffff;
            color: var(--navy);
            font-size: 14px;
            font-weight: bold;
        }

        .dataset-button {
            width: 100%;
            margin-top: 10px;
            border: 1px solid var(--blue);
            background: var(--blue);
            color: #ffffff;
        }

        .dataset-message {
            min-height: 18px;
            margin: 10px 0 0;
            color: var(--text-secondary);
            font-size: 12px;
            line-height: 1.45;
        }

        .dataset-count-grid {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 8px;
            margin-top: 13px;
        }

        .dataset-count-item {
            padding: 10px;
            border: 1px solid #e4e9f0;
            border-radius: 9px;
            background: #ffffff;
        }

        .dataset-count-label {
            display: block;
            color: var(--text-secondary);
            font-size: 11px;
        }

        .dataset-count-value {
            display: block;
            margin-top: 4px;
            color: var(--navy);
            font-size: 18px;
            font-weight: bold;
        }

        .dataset-latest {
            margin-top: 13px;
            padding-top: 12px;
            border-top: 1px solid var(--border);
        }

        .dataset-latest-file {
            margin: 0;
            color: var(--navy);
            font-size: 12px;
            font-weight: bold;
            overflow-wrap: anywhere;
        }

        .dataset-latest-details {
            margin: 5px 0 0;
            color: var(--text-secondary);
            font-size: 12px;
            line-height: 1.45;
        }

        .slogan {
            margin-top: 24px;
            padding: 24px 15px;
            border-left: 5px solid var(--yellow);
            border-radius: 12px;
            background: #fffaf0;
            text-align: center;
        }

        .slogan p {
            margin: 0;
            color: var(--navy);
            font-size: 23px;
            font-weight: bold;
            letter-spacing: 0.3px;
        }

        @media (max-width: 820px) {
            body {
                padding: 12px;
            }

            .app {
                border-radius: 16px;
            }

            .main-layout {
                grid-template-columns: 1fr;
            }

            .side-panel {
                border-top: 1px solid var(--border);
                border-left: none;
            }
        }

        @media (max-width: 560px) {
            .header {
                align-items: flex-start;
                padding: 16px;
            }

            .online-status {
                font-size: 12px;
            }

            .device-badge {
                display: none;
            }

            .camera-section,
            .side-panel {
                padding: 14px;
            }

            .control-area {
                align-items: stretch;
                flex-direction: column;
            }

            .buttons {
                display: grid;
                grid-template-columns: repeat(3, 1fr);
            }

            button {
                padding: 10px 8px;
                font-size: 12px;
            }

            #actionMessage {
                min-height: 18px;
                text-align: center;
            }

            .guide-message {
                max-width: 88%;
                overflow: hidden;
                font-size: 11px;
                text-overflow: ellipsis;
            }

            .slogan p {
                font-size: 20px;
            }
        }
    </style>
</head>

<body>
    <main class="app">
        <header class="header">
            <div class="brand">
                <div class="brand-mark">A</div>

                <div>
                    <div class="brand-row">
                        <p class="brand-name">ALQAI</p>

                        <span class="device-badge">
                            DEVICE 01
                        </span>
                    </div>

                    <p class="product-name">
                        EmotionCam
                    </p>
                </div>
            </div>

            <div class="online-status">
                <span class="status-dot"></span>
                System online
            </div>
        </header>

        <div class="main-layout">
            <section class="camera-section">
                <div class="camera-panel">
                    <div class="camera-frame">
                        <img
                            id="cameraImage"
                            alt="Live ESP32 camera preview"
                        >

                        <div class="live-badge">
                            <span class="live-dot"></span>
                            LIVE PREVIEW
                        </div>

                        <div class="face-guide"></div>

                        <div class="guide-message">
                            Place your face inside the guide
                        </div>
                    </div>

                    <div class="control-area">
                        <div class="buttons">
                            <button
                                id="pauseButton"
                                class="pause-button"
                                type="button"
                            >
                                Pause
                            </button>

                            <button
                                id="captureButton"
                                class="capture-button"
                                type="button"
                            >
                                Capture
                            </button>

                            <button
                                id="saveButton"
                                class="save-button"
                                type="button"
                            >
                                Save to SD
                            </button>
                        </div>

                        <p id="actionMessage">
                            Connecting to the camera...
                        </p>
                    </div>
                </div>
            </section>

            <aside class="side-panel">
                <section>
                    <p class="section-title">
                        DEVICE STATUS
                    </p>

                    <div class="status-list">
                        <div class="status-row">
                            <span class="status-label">
                                Camera
                            </span>

                            <span class="status-value">
                                <span
                                    id="cameraDot"
                                    class="status-dot"
                                ></span>

                                <span id="cameraStatus">
                                    Checking...
                                </span>
                            </span>
                        </div>

                        <div class="status-row">
                            <span class="status-label">
                                SD card
                            </span>

                            <span class="status-value">
                                <span
                                    id="sdDot"
                                    class="status-dot"
                                ></span>

                                <span id="sdStatus">
                                    Checking...
                                </span>
                            </span>
                        </div>

                        <div class="status-row">
                            <span class="status-label">
                                Free PSRAM
                            </span>

                            <span
                                id="psramStatus"
                                class="status-value"
                            >
                                Checking...
                            </span>
                        </div>

                        <div class="status-row">
                            <span class="status-label">
                                Free heap
                            </span>

                            <span
                                id="heapStatus"
                                class="status-value"
                            >
                                Checking...
                            </span>
                        </div>

                        <div class="status-row">
                            <span class="status-label">
                                Resolution
                            </span>

                            <span class="status-value">
                                640 × 480
                            </span>
                        </div>
                    </div>
                </section>

                <section class="capture-information">
                    <p class="section-title">
                        LATEST CAPTURE
                    </p>

                    <div class="latest-capture">
                        <p
                            id="latestFile"
                            class="latest-file"
                        >
                            No saved image yet
                        </p>

                        <p
                            id="latestDetails"
                            class="latest-details"
                        >
                            Press “Save to SD” to store a
                            JPEG image.
                        </p>
                    </div>
                </section>

                <section class="dataset-section">
                    <p class="section-title">
                        DATASET COLLECTION
                    </p>

                    <div class="dataset-card">
                        <label
                            class="dataset-label"
                            for="datasetLabel"
                        >
                            EXPRESSION LABEL
                        </label>

                        <select
                            id="datasetLabel"
                            class="dataset-select"
                        >
                            <option value="neutral">Neutral</option>
                            <option value="happy">Happy</option>
                            <option value="sad">Sad</option>
                            <option value="surprise">Surprise</option>
                        </select>

                        <button
                            id="datasetCaptureButton"
                            class="dataset-button"
                            type="button"
                        >
                            Capture Dataset Image
                        </button>

                        <p
                            id="datasetMessage"
                            class="dataset-message"
                        >
                            Select a label and keep your face
                            inside the guide.
                        </p>

                        <div class="dataset-count-grid">
                            <div class="dataset-count-item">
                                <span class="dataset-count-label">
                                    Neutral
                                </span>
                                <span
                                    id="datasetNeutralCount"
                                    class="dataset-count-value"
                                >0</span>
                            </div>

                            <div class="dataset-count-item">
                                <span class="dataset-count-label">
                                    Happy
                                </span>
                                <span
                                    id="datasetHappyCount"
                                    class="dataset-count-value"
                                >0</span>
                            </div>

                            <div class="dataset-count-item">
                                <span class="dataset-count-label">
                                    Sad
                                </span>
                                <span
                                    id="datasetSadCount"
                                    class="dataset-count-value"
                                >0</span>
                            </div>

                            <div class="dataset-count-item">
                                <span class="dataset-count-label">
                                    Surprise
                                </span>
                                <span
                                    id="datasetSurpriseCount"
                                    class="dataset-count-value"
                                >0</span>
                            </div>
                        </div>

                        <div class="dataset-latest">
                            <p
                                id="latestDatasetFile"
                                class="dataset-latest-file"
                            >
                                No dataset image saved yet
                            </p>

                            <p
                                id="latestDatasetDetails"
                                class="dataset-latest-details"
                            >
                                Files will be stored under
                                /dataset/&lt;label&gt;/.
                            </p>
                        </div>
                    </div>
                </section>

                <section class="slogan">
                    <p>Explore. Learn. Build.</p>
                </section>
            </aside>
        </div>
    </main>

    <script>
        const cameraImage =
            document.getElementById("cameraImage");

        const pauseButton =
            document.getElementById("pauseButton");

        const captureButton =
            document.getElementById("captureButton");

        const saveButton =
            document.getElementById("saveButton");

        const actionMessage =
            document.getElementById("actionMessage");

        const latestFile =
            document.getElementById("latestFile");

        const latestDetails =
            document.getElementById("latestDetails");

        const datasetLabel =
            document.getElementById("datasetLabel");

        const datasetCaptureButton =
            document.getElementById(
                "datasetCaptureButton"
            );

        const datasetMessage =
            document.getElementById("datasetMessage");

        const latestDatasetFile =
            document.getElementById(
                "latestDatasetFile"
            );

        const latestDatasetDetails =
            document.getElementById(
                "latestDatasetDetails"
            );

        const streamUrl =
            "http://" +
            window.location.hostname +
            ":81/stream";

        let previewRunning = false;
        let streamStartTimer = null;

        async function notifyOled(action) {
            try {
                await fetch(
                    "/ui-event?action=" +
                    encodeURIComponent(action),
                    { cache: "no-store" }
                );
            } catch (error) {
                console.log(
                    "OLED event could not be delivered:",
                    action
                );
            }
        }

        function startLiveStream(notifyDisplay = true) {
            previewRunning = true;

            pauseButton.textContent = "Pause";

            actionMessage.textContent =
                "Connecting to live video...";

            clearTimeout(streamStartTimer);

            if (notifyDisplay) {
                notifyOled("resume");
            }

            streamStartTimer = setTimeout(() => {
                if (!previewRunning) {
                    return;
                }

                cameraImage.src =
                    streamUrl + "?t=" + Date.now();

                actionMessage.textContent =
                    "Live stream running";
            }, 350);
        }

        function stopLiveStream(notifyDisplay = true) {
            previewRunning = false;

            clearTimeout(streamStartTimer);
            cameraImage.removeAttribute("src");

            pauseButton.textContent = "Resume";

            actionMessage.textContent =
                "Live stream paused";

            if (notifyDisplay) {
                notifyOled("pause");
            }
        }

        pauseButton.addEventListener(
            "click",
            () => {
                if (previewRunning) {
                    stopLiveStream(true);
                } else {
                    startLiveStream(true);
                }
            }
        );

        captureButton.addEventListener(
            "click",
            () => {
                // Do not send a separate PAUSE OLED event here.
                // The /capture route displays CAPTURE and CAPTURED.
                stopLiveStream(false);

                setTimeout(() => {
                    cameraImage.src =
                        "/capture?t=" + Date.now();

                    actionMessage.textContent =
                        "Snapshot captured — press Resume "
                        + "to return to live video";
                }, 300);
            }
        );

        saveButton.addEventListener(
            "click",
            async () => {
                saveButton.disabled = true;
                captureButton.disabled = true;

                actionMessage.textContent =
                    "Capturing and saving...";

                try {
                    const response =
                        await fetch(
                            "/save",
                            { cache: "no-store" }
                        );

                    const result =
                        await response.json();

                    if (
                        !response.ok ||
                        !result.success
                    ) {
                        throw new Error(
                            result.message ||
                            "Save operation failed"
                        );
                    }

                    actionMessage.textContent =
                        "Image saved successfully";

                    latestFile.textContent =
                        result.file;

                    latestDetails.textContent =
                        result.bytes +
                        " bytes stored on the SD card";
                } catch (error) {
                    actionMessage.textContent =
                        "Save error: " +
                        error.message;
                } finally {
                    saveButton.disabled = false;
                    captureButton.disabled = false;
                }
            }
        );

        function applyDatasetCounts(data) {
            document.getElementById(
                "datasetNeutralCount"
            ).textContent = data.neutral;

            document.getElementById(
                "datasetHappyCount"
            ).textContent = data.happy;

            document.getElementById(
                "datasetSadCount"
            ).textContent = data.sad;

            document.getElementById(
                "datasetSurpriseCount"
            ).textContent = data.surprise;
        }

        datasetCaptureButton.addEventListener(
            "click",
            async () => {
                const selectedLabel =
                    datasetLabel.value;

                datasetCaptureButton.disabled = true;
                saveButton.disabled = true;
                captureButton.disabled = true;
                datasetLabel.disabled = true;

                datasetMessage.textContent =
                    "Capturing " + selectedLabel +
                    " dataset image...";

                try {
                    const response = await fetch(
                        "/dataset/capture?label=" +
                        encodeURIComponent(selectedLabel),
                        {
                            method: "POST",
                            cache: "no-store"
                        }
                    );

                    const result =
                        await response.json();

                    if (
                        !response.ok ||
                        !result.success
                    ) {
                        throw new Error(
                            result.message ||
                            "Dataset capture failed"
                        );
                    }

                    applyDatasetCounts(result);

                    latestDatasetFile.textContent =
                        result.file;

                    latestDatasetDetails.textContent =
                        result.bytes +
                        " bytes — image " +
                        String(result.index).padStart(4, "0");

                    datasetMessage.textContent =
                        selectedLabel +
                        " image saved successfully";
                } catch (error) {
                    datasetMessage.textContent =
                        "Dataset error: " +
                        error.message;
                } finally {
                    datasetCaptureButton.disabled = false;
                    saveButton.disabled = false;
                    captureButton.disabled = false;
                    datasetLabel.disabled = false;
                }
            }
        );

        async function updateDatasetStatus() {
            try {
                const response = await fetch(
                    "/dataset/status",
                    { cache: "no-store" }
                );

                if (!response.ok) {
                    throw new Error(
                        "Dataset status request failed"
                    );
                }

                const status =
                    await response.json();

                applyDatasetCounts(status);

                if (status.lastFile) {
                    latestDatasetFile.textContent =
                        status.lastFile;

                    latestDatasetDetails.textContent =
                        "Most recent image saved during " +
                        "this device session";
                }

                if (!status.ready) {
                    datasetMessage.textContent =
                        "Dataset storage is not ready";
                }
            } catch (error) {
                datasetMessage.textContent =
                    "Dataset status unavailable";
            }
        }

        async function updateStatus() {
            try {
                const response =
                    await fetch(
                        "/status",
                        { cache: "no-store" }
                    );

                if (!response.ok) {
                    throw new Error(
                        "Status request failed"
                    );
                }

                const status =
                    await response.json();

                document.getElementById(
                    "cameraStatus"
                ).textContent =
                    status.stream
                        ? "Streaming"
                        : "Ready";

                document.getElementById(
                    "sdStatus"
                ).textContent =
                    status.sd
                        ? "Mounted"
                        : "Error";

                document.getElementById(
                    "heapStatus"
                ).textContent =
                    Math.round(
                        status.heap / 1024
                    ) + " KB";

                document.getElementById(
                    "psramStatus"
                ).textContent =
                    (
                        status.psram /
                        1024 /
                        1024
                    ).toFixed(2) + " MB";

                document.getElementById(
                    "cameraDot"
                ).classList.toggle(
                    "error",
                    !status.camera
                );

                document.getElementById(
                    "sdDot"
                ).classList.toggle(
                    "error",
                    !status.sd
                );
            } catch (error) {
                actionMessage.textContent =
                    "Device status unavailable";
            }
        }

        cameraImage.addEventListener(
            "error",
            () => {
                if (previewRunning) {
                    actionMessage.textContent =
                        "Live-stream connection error";
                }
            }
        );

        window.addEventListener(
            "load",
            () => {
                // Opening / already changes the OLED from the
                // Wi-Fi information screen to WEB CONNECTED.
                startLiveStream(false);
                updateStatus();
                updateDatasetStatus();
            }
        );

        window.addEventListener(
            "beforeunload",
            () => {
                clearTimeout(streamStartTimer);
                cameraImage.removeAttribute("src");
            }
        );

        setInterval(updateStatus, 2500);
        setInterval(updateDatasetStatus, 5000);
    </script>
</body>
</html>
)HTML";
}
