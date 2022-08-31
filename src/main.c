#include <stdio.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "dht11.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "mqtt.h"
#include "nvs_flash.h"
#include "wifi.h"

#define GPIO_DHT11 2

SemaphoreHandle_t connectionWifiSemaphore;
SemaphoreHandle_t connectionMQTTSemaphore;

xQueueHandle temperatureQueue;

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

void dht11Read(void *pvParams)
{

  DHT11_init(GPIO_DHT11);

  while (1)
  {
    int temp = DHT11_read().temperature;
    ESP_LOGE("Sensor", "temperatura %d", temp);
    long response = xQueueSend(temperatureQueue, &temp, 1000 / portTICK_PERIOD_MS);
    if (response)
    {
      ESP_LOGI("Leitura", "Temperatura adicionada à fila,temperatura");
    }
    else
    {
      ESP_LOGE("Leitura", "Falha do envio da temperatura");
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}
void handleCommunicationWithBroker(void *pvParams)
{
  int temperatura = 0;
  while (true)
  {
    char payload[20];
    if (xSemaphoreTake(connectionMQTTSemaphore, portMAX_DELAY))
    {
      while (true)
      {
        if (xQueueReceive(temperatureQueue, &temperatura, 3000 / portTICK_PERIOD_MS))
        {
          sprintf(payload, "temeperatura: %d", temperatura);

          publish_data("topic/teste", payload);
        }
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
  temperatureQueue = xQueueCreate(5, sizeof(float));
  xTaskCreatePinnedToCore(&ConectionWifi, "WIFI", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(&handleCommunicationWithBroker, "MQTT", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(&dht11Read, "DHT11", 4096, NULL, 1, NULL, 1);
}