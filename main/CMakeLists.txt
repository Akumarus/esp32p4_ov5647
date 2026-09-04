set (srcs 
    "main.cpp"
    "app/bmp.cpp"
    "app/isp.cpp"
    "app/csi.cpp"
    "app/ldo.cpp"
    "app/i2c.cpp"
    "app/vfs.cpp"
    "app/jpeg.cpp"
    "app/sensor.cpp"
    "app/ethernet.cpp"
)

idf_component_register(
    SRCS 
        ${srcs}
    INCLUDE_DIRS 
        "."
        "./app"
    REQUIRES
        esp_driver_jpeg
        esp_driver_cam
        esp_cam_sensor
        esp_sccb_intf
        esp_eth
        esp_netif
        driver
        sdmmc
        fatfs
        vfs
)

