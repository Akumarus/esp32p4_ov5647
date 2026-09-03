set (srcs 
    "main.cpp"
    "app/bmp.cpp"
    "app/isp.cpp"
    "app/csi.cpp"
    "app/sensor.cpp"
)

idf_component_register(
    SRCS 
        ${srcs}
    INCLUDE_DIRS 
        "."
        "./app"
    REQUIRES 
        esp_cam_sensor
        esp_sccb_intf
        driver
        esp_driver_cam
        sdmmc
        fatfs
        vfs
)

