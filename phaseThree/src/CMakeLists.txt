# Source list for the main application component:
set(app_sources
    AiModelModule.cpp
    CameraModule.cpp
    DatasetModule.cpp
    EmotionInferenceModule.cpp
    ImagePreprocessingModule.cpp
    OledModule.cpp
    SdCardModule.cpp
    StreamServerModule.cpp
    WebPage.cpp
    WebServerModule.cpp
    WifiModule.cpp
    main.cpp
)

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "../include"
    REQUIRES framework-arduinoespressif32
)
