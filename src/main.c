#include <stdio.h>
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "wifi.h"
#include "mqtt.h"
#include "freertos/semphr.h"

SemaphoreHandle_t connectionWifiSemaphore;
SemaphoreHandle_t connectionMQTTSemaphore;

void ConectionWifi(void *pvParams)
{
  while (true)
  {
    if (xSemaphoreTake(connectionWifiSemaphore, portMAX_DELAY))
    {
      mqtt_start();
    }
  }
}

void handleCommunicationWithBroker(void *pvParams)
{
  while (true)
  {
    char payload[50];
    if (xSemaphoreTake(connectionMQTTSemaphore, portMAX_DELAY))
    {
      while (true)
      {
        float temp = 20.0 * (float)rand() / (float)(RAND_MAX / 10.0);
        sprintf(payload, "temeperatura: %f", temp);
        publish_data("topic/teste", payload);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
      }
    }
  }
}

void app_main(void)
{
  // NVS é a partição da memória Flash do ESP32 responsável por armazenar dados
  // permanentes (tamanho total da partição: 24kB)

  // Inicializa o NVS
  // declaramos uma variavel do tipo esp_err_t denominada ret
  esp_err_t ret = nvs_flash_init();
  // Dentro do if nos vamos verificar se o erro é referente ESP_ERR_NVS_NO_FREE_PAGES e ESP_ERR_NVS_NEW_VERSION_FOUND
  // ESP_ERR_NVS_NO_FREE_PAGES
  // Esse erro pode acontecer se a partição NVS foi truncada
  // REFERENCIAS
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html
  // https://esp32.com/viewtopic.php?t=19422
  // ESP_ERR_NVS_NEW_VERSION_FOUND
  // A partição NVS contém dados em novo formato e não pode ser reconhecida por esta versão de código
  // REFERENCIAS
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html
  // ESP_ERROR_CHECK irá verificar se o esp_err_t é igual a ESP_OK, se o valor nao for igual uma
  // mensagem será impressa no console e o abort() será chamado.
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  connectionWifiSemaphore = xSemaphoreCreateBinary();
  connectionMQTTSemaphore = xSemaphoreCreateBinary();
  wifi_start();
  xTaskCreate(&ConectionWifi, "WIFI", 4096, NULL, 1, NULL);
  xTaskCreate(&handleCommunicationWithBroker, "MQTT", 4096, NULL, 1, NULL);
}