# type: ignore
# This script is executed by PlatformIO's SCons build system, which dynamically
# injects 'Import' and 'env' variables. We use '# type: ignore' to prevent
# VS Code's Pylance/linter from reporting false positive undefined variable warnings.
Import("env")

# esp32-camera 2.0.4's PlatformIO manifest omits its target-specific sources.
# Add the ESP32-S3 low-level camera and XCLK implementations explicitly.
camera_root = env.subst(
    "$PROJECT_LIBDEPS_DIR/$PIOENV/esp32-camera"
)

env.Append(
    CPPPATH=[
        camera_root,
        camera_root + "/driver/include",
        camera_root + "/driver/private_include",
        camera_root + "/conversions/include",
        camera_root + "/conversions/private_include",
        camera_root + "/sensors/private_include",
        camera_root + "/target/private_include",
    ]
)

env.BuildSources(
    env.subst("$BUILD_DIR/camera-target"),
    camera_root + "/target",
    src_filter=[
        "-<*>",
        "+<esp32s3/ll_cam.c>",
    ],
)
