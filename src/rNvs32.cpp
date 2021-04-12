#include "rNvs32.h"
#include "rStrings.h"
#include "project_config.h"
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include "rLog.h"
#include "sys/queue.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "nvs_handle.hpp"

static const char * tagNVS = "NVS";

uint16_t string2time(const char* str_value)
{
  uint h, m;
  char c;
  sscanf(str_value, CONFIG_FORMAT_TIMEINT_SCAN, &h, &c, &m);
  if (h>23) h=23;
  if (m>59) m=59;
  return 100 * h + m;
}

char* time2string(uint16_t time)
{
  return malloc_stringf(CONFIG_FORMAT_TIMEINT, time / 100, time % 100);
}

uint32_t string2timespan(const char* str_value)
{
  uint h1, m1, h2, m2;
  char c1, c2, c3;
  sscanf(str_value, CONFIG_FORMAT_TIMESPAN_SCAN, &h1, &c1, &m1, &c2, &h2, &c3, &m2);
  if (h1>23) h1=23;
  if (m1>59) m1=59;
  if (h2>23) h2=23;
  if (m2>59) m2=59;
  return 10000 * (100 * h1 + m1) + (100 * h2 + m2);
}

char* timespan2string(uint32_t timespan)
{
  uint16_t t1 = timespan / 10000;
  uint16_t t2 = timespan % 10000;
  return malloc_stringf(CONFIG_FORMAT_TIMESPAN, t1 / 100, t1 % 100, t2 / 100, t2 % 100); 
}

bool checkTimespan(time_t time, uint32_t timespan)
{
  static struct tm ti; 
  localtime_r(&time, &ti);
  int16_t  t0 = ti.tm_hour * 100 + ti.tm_min;
  uint16_t t1 = timespan / 10000;
  uint16_t t2 = timespan % 10000;

  // t1 < t2 :: ((t0 >= t1) && (t0 < t2))
  // t0=0559 t1=0600 t2=2300 : (0559 >= 0600) && (0559 < 2300) = 0 && 1 = 0
  // t0=0600 t1=0600 t2=2300 : (0600 >= 0600) && (0600 < 2300) = 1 && 1 = 1
  // t0=0601 t1=0600 t2=2300 : (0601 >= 0600) && (0601 < 2300) = 1 && 1 = 1
  // t0=2259 t1=0600 t2=2300 : (2259 >= 0600) && (2259 < 2300) = 1 && 1 = 1
  // t0=2300 t1=0600 t2=2300 : (2300 >= 0600) && (2300 < 2300) = 1 && 0 = 0
  // t0=2301 t1=0600 t2=2300 : (2301 >= 0600) && (2301 < 2300) = 1 && 0 = 0

  // t1 > t2 :: !((t0 >= t2) && (t1 > t0))
  // t0=2259 t1=2300 t2=0600 : (2259 >= 0600) && (2300 > 2259) = 1 && 1 = 1 !=> 0
  // t0=2300 t1=2300 t2=0600 : (2300 >= 0600) && (2300 > 2300) = 1 && 0 = 0 !=> 1
  // t0=2301 t1=2300 t2=0600 : (2301 >= 0600) && (2300 > 2301) = 1 && 0 = 0 !=> 1
  // t0=0559 t1=2300 t2=0600 : (0559 >= 0600) && (2300 > 0559) = 0 && 1 = 0 !=> 1
  // t0=0600 t1=2300 t2=0600 : (0600 >= 0600) && (2300 > 0600) = 1 && 1 = 1 !=> 0
  // t0=0601 t1=2300 t2=0600 : (0601 >= 0600) && (2300 > 0601) = 1 && 1 = 1 !=> 0

  return (t1 < t2) ? ((t0 >= t1) && (t0 < t2)) : !((t0 >= t2) && (t1 > t0));
}

char* value2string(const param_type_t type_value, void *value)
{
  if (value) {
    switch (type_value) {
      case OPT_TYPE_I8:
        return malloc_stringf("%d", *(int8_t*)value);
      case OPT_TYPE_U8:
        return malloc_stringf("%d", *(uint8_t*)value);
      case OPT_TYPE_I16:
        return malloc_stringf("%d", *(int16_t*)value);
      case OPT_TYPE_U16:
        return malloc_stringf("%d", *(uint16_t*)value);
      case OPT_TYPE_I32:
        return malloc_stringf("%d", *(int32_t*)value);
      case OPT_TYPE_U32:
        return malloc_stringf("%d", *(uint32_t*)value);
      case OPT_TYPE_I64:
        return malloc_stringf("%d", *(int64_t*)value);
      case OPT_TYPE_U64:
        return malloc_stringf("%d", *(uint64_t*)value);
      case OPT_TYPE_FLOAT:
        return malloc_stringf("%f", *(float*)value);
      case OPT_TYPE_DOUBLE:
        return malloc_stringf("%f", *(double*)value);
      case OPT_TYPE_STRING:
        return strdup((char*)value);
      case OPT_TYPE_TIME:
        return time2string(*(uint16_t*)value);;
      case OPT_TYPE_TIMESPAN:
        return timespan2string(*(uint32_t*)value);
      default:
        return nullptr;
    };
  };
  return nullptr;
}

void* string2value(const param_type_t type_value, char* str_value)
{
  void* value = nullptr;
  switch (type_value) {
    case OPT_TYPE_I8:
      value = malloc(sizeof(int8_t));
      *(int8_t*)value = (int8_t)strtoimax(str_value, nullptr, 0);
      break;
    case OPT_TYPE_U8:
      value = malloc(sizeof(uint8_t));
      *(uint8_t*)value = (uint8_t)strtoumax(str_value, nullptr, 0);
      break;
    case OPT_TYPE_I16:
      value = malloc(sizeof(int16_t));
      *(int16_t*)value = (int16_t)strtoimax(str_value, nullptr, 0);
      break;
    case OPT_TYPE_U16:
      value = malloc(sizeof(uint16_t));
      *(uint16_t*)value = (uint16_t)strtoumax(str_value, nullptr, 0);
      break;
    case OPT_TYPE_I32:
      value = malloc(sizeof(int32_t));
      *(int32_t*)value = (int32_t)strtoimax(str_value, nullptr, 0);
      break;
    case OPT_TYPE_U32:
      value = malloc(sizeof(uint32_t));
      *(uint32_t*)value = (uint32_t)strtoumax(str_value, nullptr, 0);
      break;
    case OPT_TYPE_I64:
      value = malloc(sizeof(int64_t));
      *(uint64_t*)value = (uint64_t)strtoumax(str_value, nullptr, 0);
      break;
    case OPT_TYPE_U64:
      value = malloc(sizeof(uint64_t));
      *(uint64_t*)value = (uint64_t)strtoumax(str_value, nullptr, 0);
      break;
    case OPT_TYPE_FLOAT:
      value = malloc(sizeof(float));
      *(float*)value = (float)strtof(str_value, nullptr);
      break;
    case OPT_TYPE_DOUBLE:
      value = malloc(sizeof(double));
      *(double*)value = (double)strtod(str_value, nullptr);
      break;
    case OPT_TYPE_STRING:
      value = strdup(str_value);
      break;
    case OPT_TYPE_TIME:
      value = malloc(sizeof(uint16_t));
      *(uint16_t*)value = string2time(str_value);
      break;
    case OPT_TYPE_TIMESPAN:
      value = malloc(sizeof(uint32_t));
      *(uint32_t*)value = string2timespan(str_value);
      break;
    default:
      return nullptr;
  };
  return value;
}

bool equal2value(const param_type_t type_value, void *value1, void *value2)
{
  if ((value1) && (value2)) {
    switch (type_value) {
      case OPT_TYPE_I8:
        return *(int8_t*)value1 == *(int8_t*)value2;
      case OPT_TYPE_U8:
        return *(uint8_t*)value1 == *(uint8_t*)value2;
      case OPT_TYPE_I16:
        return *(int16_t*)value1 == *(int16_t*)value2;
      case OPT_TYPE_U16:
        return *(uint16_t*)value1 == *(uint16_t*)value2;
      case OPT_TYPE_I32:
        return *(int32_t*)value1 == *(int32_t*)value2;
      case OPT_TYPE_U32:
        return *(uint32_t*)value1 == *(uint32_t*)value2;
      case OPT_TYPE_I64:
        return *(int64_t*)value1 == *(int64_t*)value2;
      case OPT_TYPE_U64:
        return *(uint64_t*)value1 == *(uint64_t*)value2;
      case OPT_TYPE_FLOAT:
        return *(float*)value1 == *(float*)value2;
      case OPT_TYPE_DOUBLE:
        return *(double*)value1 == *(double*)value2;
      case OPT_TYPE_STRING:
        return strcmp((char*)value1, (char*)value2) == 0;
      case OPT_TYPE_TIME:
        return *(uint16_t*)value1 == *(uint16_t*)value2;
      case OPT_TYPE_TIMESPAN:
        return *(uint32_t*)value1 == *(uint32_t*)value2;
      default:
        return false;
    };
  }
  else {
    if ((!value1) && (!value2)) {
      return true;
    } else {
      return false;
    };
  };
}

void setNewValue(const param_type_t type_value, void *value1, void *value2)
{
  switch (type_value) {
    case OPT_TYPE_I8:
      *(int8_t*)value1 = *(int8_t*)value2;
      return;
    case OPT_TYPE_U8:
      *(uint8_t*)value1 = *(uint8_t*)value2;
      return;
    case OPT_TYPE_I16:
      *(int16_t*)value1 = *(int16_t*)value2;
      return;
    case OPT_TYPE_U16:
      *(uint16_t*)value1 = *(uint16_t*)value2;
      return;
    case OPT_TYPE_I32:
      *(int32_t*)value1 = *(int32_t*)value2;
      return;
    case OPT_TYPE_U32:
      *(uint32_t*)value1 = *(uint32_t*)value2;
      return;
    case OPT_TYPE_I64:
      *(int64_t*)value1 = *(int64_t*)value2;
      return;
    case OPT_TYPE_U64:
      *(uint64_t*)value1 = *(uint64_t*)value2;
      return;
    case OPT_TYPE_FLOAT:
      *(float*)value1 = *(float*)value2;
      return;
    case OPT_TYPE_DOUBLE:
      *(double*)value1 = *(double*)value2;
      return;
    case OPT_TYPE_STRING:
      if (value1) free(value1);
      value1 = strdup((char*)value2);
      return;
    case OPT_TYPE_TIME:
      *(uint16_t*)value1 = *(uint16_t*)value2;
      return;
    case OPT_TYPE_TIMESPAN:
      *(uint32_t*)value1 = *(uint32_t*)value2;
      return;
    default:
      return;
  };

}

bool nvsInit()
{
  esp_err_t err = nvs_flash_init();
  if ((err == ESP_ERR_NVS_NO_FREE_PAGES) || (err == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
    rlog_i(tagNVS, "Erasing NVS partition...");
    nvs_flash_erase();
    err = nvs_flash_init();
  };
  if (err != ESP_OK) {
    rlog_e(tagNVS, "NVS partition initialization error: %d (%s)", err, esp_err_to_name(err));
    return false;
  }
  else {
    rlog_i(tagNVS, "NVS partition initilized");
    return true;
  };
}

bool nvsOpen(const char* name_group, nvs_open_mode_t open_mode, nvs_handle_t *nvs_handle)
{
  esp_err_t err = nvs_open(name_group, open_mode, nvs_handle); 
  if (err != ESP_OK) {
    rlog_e(tagNVS, "Error opening NVS namespace \"%s\": %d (%s)!", name_group, err, esp_err_to_name(err));
    return false;
  };
  return true;
}

bool nvsRead(const char* name_group, const char* name_key, const param_type_t type_value, void * value)
{
  nvs_handle_t nvs_handle;
  // Open NVS namespace
  if (!nvsOpen(name_group, NVS_READONLY, &nvs_handle)) return false;

  // Read value
  esp_err_t err = ESP_OK;
  if (type_value == OPT_TYPE_STRING) {
    // Get the size of the string that is in the storage
    size_t new_len = 0;
    err = nvs_get_str(nvs_handle, name_key, NULL, &new_len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
      rlog_d(tagNVS, "Value \"%s.%s\" is not initialized yet, used default: [%s]", name_group, name_key, (char*)value);
    }
    else {
      if (err != ESP_OK) {
        rlog_w(tagNVS, "Error reading string \"%s.%s\": %d (%s)!", name_group, name_key, err, esp_err_to_name(err));
      };
    }

    // If the result of reading the length is successful, read the line itself
    if (err == ESP_OK) {
      size_t old_len = strlen((char*)value)+1;
      char* prev_value = nullptr;
      // Allocate a new block of memory if the size of the string is different
      if (new_len != old_len) {
        // Let's save the pointer to the previous data for now (in case of failure)
        prev_value = (char*)value;
        value = malloc(new_len);
      };
      // Reading a line from storage
      err = nvs_get_str(nvs_handle, name_key, (char*)value, &new_len);
      if (err == ESP_OK) {
        // It's okay, delete the previous value
        if (prev_value) {
          free(prev_value);
        };
        rlog_d(tagNVS, "Read string value \"%s.%s\": [%s]", name_group, name_key, (char*)value);
      } else {
        // We delete the allocated memory for new data and return the previous value
        if (prev_value) {
          free(value);
          value = prev_value;
          prev_value = nullptr;
        };
        rlog_e(tagNVS, "Error reading string \"%s.%s\": %d (%s)!", name_group, name_key, err, esp_err_to_name(err));
      };
    };
  } else {
    size_t data_len = 0;
    switch (type_value) {
      case OPT_TYPE_I8:
        err = nvs_get_i8(nvs_handle, name_key, (int8_t*)value);
        break;
      case OPT_TYPE_U8:
        err = nvs_get_u8(nvs_handle, name_key, (uint8_t*)value);
        break;
      case OPT_TYPE_I16:
        err = nvs_get_i16(nvs_handle, name_key, (int16_t*)value);
        break;
      case OPT_TYPE_U16:
        err = nvs_get_u16(nvs_handle, name_key, (uint16_t*)value);
        break;
      case OPT_TYPE_I32:
        err = nvs_get_i32(nvs_handle, name_key, (int32_t*)value);
        break;
      case OPT_TYPE_U32:
        err = nvs_get_u32(nvs_handle, name_key, (uint32_t*)value);
        break;
      case OPT_TYPE_I64:
        err = nvs_get_i64(nvs_handle, name_key, (int64_t*)value);
        break;
      case OPT_TYPE_U64:
        err = nvs_get_u64(nvs_handle, name_key, (uint64_t*)value);
        break;
      case OPT_TYPE_FLOAT:
        data_len = sizeof(float);
        err = nvs_get_blob(nvs_handle, name_key, (float*)value, &data_len);
        break;
      case OPT_TYPE_DOUBLE:
        data_len = sizeof(double);
        err = nvs_get_blob(nvs_handle, name_key, (double*)value, &data_len);
        break;
      case OPT_TYPE_TIME:
        err = nvs_get_u16(nvs_handle, name_key, (uint16_t*)value);
        break;
      case OPT_TYPE_TIMESPAN:
        err = nvs_get_u32(nvs_handle, name_key, (uint32_t*)value);
        break;
      default:
        err = ESP_ERR_NVS_TYPE_MISMATCH;
        break;
    };
    
    char* str_value = value2string(type_value, value);
    switch (err) {
      case ESP_OK:
        rlog_d(tagNVS, "Read value \"%s.%s\": [%s]", name_group, name_key, str_value);
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        rlog_d(tagNVS, "Value \"%s.%s\" is not initialized yet, used default: [%s]", name_group, name_key, str_value);
        break;
      default :
        rlog_e(tagNVS, "Error reading \"%s.%s\": %d (%s)!", name_group, name_key, err, esp_err_to_name(err));
    };
    free(str_value);
  };

  nvs_close(nvs_handle);
  return (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND);
}

bool nvsWrite(const char* name_group, const char* name_key, const param_type_t type_value, void * value)
{
  nvs_handle_t nvs_handle;
  // Open NVS namespace
  if (!nvsOpen(name_group, NVS_READWRITE, &nvs_handle)) return false;

  // Write value
  esp_err_t err = ESP_OK;
  switch (type_value) {
    case OPT_TYPE_I8:
      err = nvs_set_i8(nvs_handle, name_key, *(int8_t*)value);
      break;
    case OPT_TYPE_U8:
      err = nvs_set_u8(nvs_handle, name_key, *(uint8_t*)value);
      break;
    case OPT_TYPE_I16:
      err = nvs_set_i16(nvs_handle, name_key, *(int16_t*)value);
      break;
    case OPT_TYPE_U16:
      err = nvs_set_u16(nvs_handle, name_key, *(uint16_t*)value);
      break;
    case OPT_TYPE_I32:
      err = nvs_set_i32(nvs_handle, name_key, *(int32_t*)value);
      break;
    case OPT_TYPE_U32:
      err = nvs_set_u32(nvs_handle, name_key, *(uint32_t*)value);
      break;
    case OPT_TYPE_I64:
      err = nvs_set_i64(nvs_handle, name_key, *(int64_t*)value);
      break;
    case OPT_TYPE_U64:
      err = nvs_set_u64(nvs_handle, name_key, *(uint64_t*)value);
      break;
    case OPT_TYPE_FLOAT:
      err = nvs_set_blob(nvs_handle, name_key, value, sizeof(float));
      break;
    case OPT_TYPE_DOUBLE:
      err = nvs_set_blob(nvs_handle, name_key, value, sizeof(double));
      break;
    case OPT_TYPE_STRING:
      err = nvs_set_str(nvs_handle, name_key, (char*)value);
      break;
    case OPT_TYPE_TIME:
      err = nvs_set_u16(nvs_handle, name_key, *(uint16_t*)value);
      break;
    case OPT_TYPE_TIMESPAN:
      err = nvs_set_u32(nvs_handle, name_key, *(uint32_t*)value);
      break;
    default:
      err = ESP_ERR_NVS_TYPE_MISMATCH;
      break;
  };

  if (err == ESP_OK) {
    err = nvs_commit(nvs_handle);
  };

  if (err == ESP_OK) {
    rlog_d(tagNVS, "Value \"%s.%s\" was successfully written to storage", name_group, name_key);
  }
  else {
    rlog_e(tagNVS, "Error writting \"%s.%s\": %d (%s)!", name_group, name_key, err, esp_err_to_name(err));
  };

  nvs_close(nvs_handle);
  return (err == ESP_OK);
}


