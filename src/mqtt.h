#ifndef MQTT_H
#define MQTT_H

void mqtt_start();
void publish_data(char *topic, char *payload);
#endif