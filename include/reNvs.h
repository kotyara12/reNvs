/* 
   EN: Library for storing and managing parameters ESP32 (ESP-IDF)
   RU: Библиотека хранения и управления параметрами ESP32 (ESP-IDF)
   --------------------------
   (с) 2021 Разживин Александр | Razzhivin Alexander
   kotyara12@yandex.ru | https://kotyara12.ru | tg: @kotyara1971
*/

#ifndef __RE_NVS_H__
#define __RE_NVS_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "rTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

uint16_t string2time(const char* str_value);
char* time2string(uint16_t time);
timespan_t string2timespan(const char* str_value);
char* timespan2string(timespan_t timespan);

char* value2string(const param_type_t type_value, void *value);
void* string2value(const param_type_t type_value, char* str_value);
void* clone2value(const param_type_t type_value, void *value);
bool  equal2value(const param_type_t type_value, void *value1, void *value2);
bool  valueCheckLimits(const param_type_t type_value, void *value, void *value_min, void *value_max);
void  setNewValue(const param_type_t type_value, void *value1, void *value2);

bool nvsInit();
bool nvsOpen(const char* name_group, nvs_open_mode_t open_mode, nvs_handle_t *nvs_handle);
bool nvsRead(const char* name_group, const char* name_key, const param_type_t type_value, void * value);
bool nvsWrite(const char* name_group, const char* name_key, const param_type_t type_value, void * value);

#ifdef __cplusplus
}
#endif

#endif // __RE_NVS_H__