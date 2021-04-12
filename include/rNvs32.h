/* 
   EN: Library for storing and managing parameters ESP32 (ESP-IDF)
   RU: Библиотека хранения и управления параметрами ESP32 (ESP-IDF)
   --------------------------
   (с) 2021 Разживин Александр | Razzhivin Alexander
   kotyara12@yandex.ru | https://kotyara12.ru | tg: @kotyara1971
*/

#ifndef __RNVS32_H__
#define __RNVS32_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "nvs.h"
#include "nvs_flash.h"

typedef enum { 
  OPT_TYPE_PARAMETER = 0, 
  OPT_TYPE_OTA       = 1,
  OPT_TYPE_COMMAND   = 2
} param_type_t;

typedef enum { 
  OPT_VAL_UNKNOWN  = 0,
  OPT_VAL_I8       = 1,
  OPT_VAL_U8       = 2,
  OPT_VAL_I16      = 3,
  OPT_VAL_U16      = 4,
  OPT_VAL_I32      = 5,
  OPT_VAL_U32      = 6,
  OPT_VAL_I64      = 7,
  OPT_VAL_U64      = 8,
  OPT_VAL_FLOAT    = 9,
  OPT_VAL_DOUBLE   = 10,
  OPT_VAL_STRING   = 11,
  OPT_VAL_TIME     = 12,
  OPT_VAL_TIMESPAN = 13
} param_value_t;

#ifdef __cplusplus
extern "C" {
#endif

uint16_t string2time(const char* str_value);
char* time2string(uint16_t time);
uint32_t string2timespan(const char* str_value);
char* timespan2string(uint32_t timespan);
bool checkTimespan(time_t time, uint32_t timespan);

char* value2string(const param_value_t type_value, void *value);
void* string2value(const param_value_t type_value, char* str_value);
bool  equal2value(const param_value_t type_value, void *value1, void *value2);
void  setNewValue(const param_value_t type_value, void *value1, void *value2);

bool nvsInit();
bool nvsRead(const char* name_group, const char* name_key, const param_value_t type_value, void * value);
bool nvsWrite(const char* name_group, const char* name_key, const param_value_t type_value, void * value);

#ifdef __cplusplus
}
#endif

#endif // __RNVS32_H__