#include "ethernet.hpp"
#include "esp_eth_driver.h"
#include "esp_eth_netif_glue.h"
#include "esp_netif.h"
#include "esp_log.h"

Ethernet::Ethernet()
{
    eth_mac_config_t macConfig = ETH_MAC_DEFAULT_CONFIG();
    eth_esp32_emac_config_t emacConfig = {};
    emacConfig.smi_gpio.mdc_num = 31;
    emacConfig.smi_gpio.mdio_num = 52;
    emacConfig.interface = EMAC_DATA_INTERFACE_RMII;
    emacConfig.clock_config.rmii.clock_mode = EMAC_CLK_EXT_IN;
    emacConfig.clock_config.rmii.clock_gpio = (emac_rmii_clock_gpio_t)50;
    emacConfig.dma_burst_len = ETH_DMA_BURST_LEN_32;
    emacConfig.intr_priority = 0;
    emacConfig.mdc_freq_hz = 0;
    emacConfig.emac_dataif_gpio.rmii.tx_en_num = 49;
    emacConfig.emac_dataif_gpio.rmii.txd0_num = 34;
    emacConfig.emac_dataif_gpio.rmii.txd1_num = 35;
    emacConfig.emac_dataif_gpio.rmii.crs_dv_num = 28;
    emacConfig.emac_dataif_gpio.rmii.rxd0_num = 29;
    emacConfig.emac_dataif_gpio.rmii.rxd1_num = 30;
    emacConfig.clock_config_out_in.rmii.clock_mode = EMAC_CLK_EXT_IN;
    emacConfig.clock_config_out_in.rmii.clock_gpio = (emac_rmii_clock_gpio_t)-1;
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&emacConfig, &macConfig);

    eth_phy_config_t phyConfig = ETH_PHY_DEFAULT_CONFIG();
    phyConfig.phy_addr = 1;
    phyConfig.reset_gpio_num = 51;
    esp_eth_phy_t *phy = esp_eth_phy_new_ip101(&phyConfig);

    esp_eth_config_t ethConfig = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t ethHandle = nullptr;
    esp_eth_driver_install(&ethConfig, &ethHandle);

    esp_netif_init();
    esp_event_loop_create_default();
    
    esp_netif_config_t netifConfig = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *ethNetif = esp_netif_new(&netifConfig);
    esp_eth_netif_glue_handle_t ethNetifGlue = esp_eth_new_netif_glue(ethHandle);
    esp_netif_attach(ethNetif, ethNetifGlue);

    
    esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, 
                               &Ethernet::ethEventHandler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP,
                               &Ethernet::ipEventHandler, NULL);
    esp_eth_start(ethHandle);
}

void Ethernet::ethEventHandler(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData)
{
    uint8_t macAddr[6] = {};
    esp_eth_handle_t ethHandle = *static_cast<esp_eth_handle_t*>(eventData);

    switch (eventId)
    {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(ethHandle, ETH_CMD_G_MAC_ADDR, macAddr);
        ESP_LOGI("Ethernet", "Ethernet Link Up");
        ESP_LOGI("Ethernet", "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI("Ethernet", "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI("Ethernet", "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI("Ethernet", "Ethernet Stopped");
        break;
    default:
        break;
    }
}

void Ethernet::ipEventHandler(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData)
{
    ip_event_got_ip_t *event = static_cast<ip_event_got_ip_t *>(eventData);
    const esp_netif_ip_info_t *ipInfo = &event->ip_info;
    ESP_LOGI("Ethernet", "Ethernet Got IP Address");
    ESP_LOGI("Ethernet", "~~~~~~~~~~~");
    ESP_LOGI("Ethernet", "ETHIP:" IPSTR, IP2STR(&ipInfo->ip));
    ESP_LOGI("Ethernet", "ETHMASK:" IPSTR, IP2STR(&ipInfo->netmask));
    ESP_LOGI("Ethernet", "ETHGW:" IPSTR, IP2STR(&ipInfo->gw));
    ESP_LOGI("Ethernet", "~~~~~~~~~~~");
}