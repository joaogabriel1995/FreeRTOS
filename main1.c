#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_wifi.h"

#define PLED 2

SemaphoreHandle_t mutex;
SemaphoreHandle_t xSemaphoreWifi;
SemaphoreHandle_t xSemaphoreMQTT;

void connectWifi(void *pvParams)
{
  while (true)
  {
    ESP_LOGI("Wifi", "Conectado ao wifi!");
    xSemaphoreGive(xSemaphoreMQTT);
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

void connectMQTT(void *pvParams)
{
  while (true)
  {
    ESP_LOGI("MQTT", "Conectado ao MQTT!");
    xSemaphoreTake(xSemaphoreMQTT, portMAX_DELAY);
  }
}

void led(void *pvParams)
{
  gpio_pad_select_gpio(PLED);
  gpio_set_direction(PLED, GPIO_MODE_OUTPUT);

  int estado = 0;
  while (true)
  {
    if (xSemaphoreTake(mutex, 1000 / portTICK_PERIOD_MS))
    {
      gpio_set_level(PLED, estado);
      estado = !estado;
      xSemaphoreGive(mutex);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    else
    {
      ESP_LOGE("Led", "Não é possivel liigar o led");
    }
  }
}
void dht11(void *pvParams)
{
  while (true)
  {
    if (xSemaphoreTake(mutex, 1000 / portTICK_PERIOD_MS))
    {
      ESP_LOGI("dht11", "Leitura de sensor");
      xSemaphoreGive(mutex);
    }

    else
    {
      ESP_LOGE("Led", "Não foi possivel ler o sensor");
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void app_main()
{

  // Inicializa o NVS

  ESP_ERROR_CHECK(nvs_flash_init());

  // Inicializar a interface de Rede (Network Interface)
  ESP_ERROR_CHECK(esp_netif_init());

  // Inicializa a tarefa padrão de tratamento de eventos
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  xSemaphoreMQTT = xSemaphoreCreateBinary();
  mutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(&led, "led", 2040, "task3", 1, NULL, 1);
  xTaskCreatePinnedToCore(&dht11, "dht11", 2040, "task4", 1, NULL, 1);
  xTaskCreatePinnedToCore(&connectWifi, "wifi", 2040, "task1", 1, NULL, 0);
  xTaskCreatePinnedToCore(&connectMQTT, "mqtt", 2040, "task2", 1, NULL, 0);
}