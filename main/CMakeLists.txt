set (srcs 
    "main.cpp"
)

idf_component_register(
    SRCS 
        ${srcs}
    INCLUDE_DIRS 
        "."
    REQUIRES 
        esp_cam_sensor
        esp_sccb_intf
        driver
        esp_driver_cam
)

