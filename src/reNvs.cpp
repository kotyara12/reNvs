#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include "reNvs.h"
#include "rLog.h"
#include "rTypes.h"
#include "rStrings.h"
#include "reEsp32.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include "sys/queue.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "nvs_handle.hpp"
#include "project_config.h"
#include "def_consts.h"

#if CONFIG_RLOG_PROJECT_LEVEL > RLOG_LEVEL_NONE
static const char * logTAG = "NVS";
#endif // CONFIG_RLOG_PROJECT_LEVEL

esp_err_t nvs_set_float(nvs_handle_t c_handle, const char* key, float in_value)
{
  uint32_t buf = 0;
  memcpy(&buf, &in_value, sizeof(float));
  return nvs_set_u32(c_handle, key, buf);
}

esp_err_t nvs_get_float(nvs_handle_t c_handle, const char* key, float* out_value)
{
  uint32_t buf = 0;
  esp_err_t err = nvs_get_u32(c_handle, key, &buf);
  if (err == ESP_OK) {
    memcpy(out_value, &buf, sizeof(float));
  } else {
    size_t _old_mode_size = sizeof(float);
    err = nvs_get_blob(c_handle, key, out_value, &_old_mode_size);
  };
  return err;
}

esp_err_t nvs_set_double(nvs_handle_t c_handle, const char* key, double in_value)
{
  uint64_t buf = 0;
  memcpy(&buf, &in_value, sizeof(double));
  return nvs_set_u64(c_handle, key, buf);
}

esp_err_t nvs_get_double(nvs_handle_t c_handle, const char* key, double* out_value)
{
  uint64_t buf = 0;
  esp_err_t err = nvs_get_u64(c_handle, key, &buf);
  if (err == ESP_OK) {
    memcpy(out_value, &buf, sizeof(double));
  } else {
    size_t _old_mode_size = sizeof(double);
    err = nvs_get_blob(c_handle, key, out_value, &_old_mode_size);
  };
  return err;
}

esp_err_t nvs_set_time(nvs_handle_t c_handle, const char* key, time_t in_value)
{
  uint64_t buf = 0;
  memcpy(&buf, &in_value, sizeof(time_t));
  return nvs_set_u64(c_handle, key, buf);
}

esp_err_t nvs_get_time(nvs_handle_t c_handle, const char* key, time_t* out_value)
{
  uint64_t buf = 0;
  esp_err_t err = nvs_get_u64(c_handle, key, &buf);
  if (err == ESP_OK) {
    memcpy(out_value, &buf, sizeof(time_t));
  } else {
    size_t _old_mode_size = sizeof(time_t);
    err = nvs_get_blob(c_handle, key, out_value, &_old_mode_size);
  };
  return err;
}

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

timespan_t string2timespan(const char* str_value)
{
  uint h1, m1, h2, m2;
  char c1, c2, c3;
  sscanf(str_value, CONFIG_FORMAT_TIMESPAN_SCAN, &h1, &c1, &m1, &c2, &h2, &c3, &m2);
  if (h1>23) h1=23;
  if (m1>59) m1=59;
  if (h2>24) {
    h2=24;
    m2=0;
  } else {
    if (m2>59) m2=59;
  };
  return 10000 * (100 * h1 + m1) + (100 * h2 + m2);
}

char* timespan2string(timespan_t timespan)
{
  uint32_t t1 = 0;
  uint32_t t2 = 0;
  if (timespan > 0) {
    t1 = timespan / 10000;
    t2 = timespan % 10000;
  };
  return malloc_stringf(CONFIG_FORMAT_TIMESPAN, t1 / 100, t1 % 100, t2 / 100, t2 % 100); 
}

char* value2string(const param_type_t type_value, void *value)
{
  if (value) {
    switch (type_value) {
      case OPT_TYPE_I8:
        return malloc_stringf(CONFIG_FORMAT_OPT_I8, *(int8_t*)value);
      case OPT_TYPE_U8:
        return malloc_stringf(CONFIG_FORMAT_OPT_U8, *(uint8_t*)value);
      case OPT_TYPE_I16:
        return malloc_stringf(CONFIG_FORMAT_OPT_I16, *(int16_t*)value);
      case OPT_TYPE_U16:
        return malloc_stringf(CONFIG_FORMAT_OPT_U16, *(uint16_t*)value);
      case OPT_TYPE_I32:
        return malloc_stringf(CONFIG_FORMAT_OPT_I32, *(int32_t*)value);
      case OPT_TYPE_U32:
        return malloc_stringf(CONFIG_FORMAT_OPT_U32, *(uint32_t*)value);
      case OPT_TYPE_I64:
        return malloc_stringf(CONFIG_FORMAT_OPT_I64, *(int64_t*)value);
      case OPT_TYPE_U64:
        return malloc_stringf(CONFIG_FORMAT_OPT_U64, *(uint64_t*)value);
      case OPT_TYPE_FLOAT:
        return malloc_stringf(CONFIG_FORMAT_OPT_FLOAT, *(float*)value);
      case OPT_TYPE_DOUBLE:
        return malloc_stringf(CONFIG_FORMAT_OPT_DOUBLE, *(double*)value);
      case OPT_TYPE_STRING:
        return strdup((char*)value);
      case OPT_TYPE_TIMEVAL:
        return time2string(*(uint16_t*)value);
      case OPT_TYPE_TIMESPAN:
        return timespan2string(*(timespan_t*)value);
      default:
        return nullptr;
    };
  };
  return nullptr;
}

void* string2value(const param_type_t type_value, char* str_value)
{
  void* value = nullptr;
  if (str_value) {
    switch (type_value) {
      case OPT_TYPE_I8:
        value = esp_malloc(sizeof(int8_t));
        if (value) {
          *(int8_t*)value = (int8_t)strtoimax(str_value, nullptr, 0);
        };
        break;
      case OPT_TYPE_U8:
        value = esp_malloc(sizeof(uint8_t));
        if (value) {
          *(uint8_t*)value = (uint8_t)strtoumax(str_value, nullptr, 0);
        };
        break;
      case OPT_TYPE_I16:
        value = esp_malloc(sizeof(int16_t));
        if (value) {
          *(int16_t*)value = (int16_t)strtoimax(str_value, nullptr, 0);
        };
        break;
      case OPT_TYPE_U16:
        value = esp_malloc(sizeof(uint16_t));
        if (value) {
          *(uint16_t*)value = (uint16_t)strtoumax(str_value, nullptr, 0);
        };
        break;
      case OPT_TYPE_I32:
        value = esp_malloc(sizeof(int32_t));
        if (value) {
          *(int32_t*)value = (int32_t)strtoimax(str_value, nullptr, 0);
        };
        break;
      case OPT_TYPE_U32:
        value = esp_malloc(sizeof(uint32_t));
        if (value) {
          *(uint32_t*)value = (uint32_t)strtoumax(str_value, nullptr, 0);
        };
        break;
      case OPT_TYPE_I64:
        value = esp_malloc(sizeof(int64_t));
        if (value) {
          *(uint64_t*)value = (uint64_t)strtoumax(str_value, nullptr, 0);
        };
        break;
      case OPT_TYPE_U64:
        value = esp_malloc(sizeof(uint64_t));
        if (value) {
          *(uint64_t*)value = (uint64_t)strtoumax(str_value, nullptr, 0);
        };
        break;
      case OPT_TYPE_FLOAT:
        value = esp_malloc(sizeof(float));
        if (value) {
          *(float*)value = (float)strtof(str_value, nullptr);
        };
        break;
      case OPT_TYPE_DOUBLE:
        value = esp_malloc(sizeof(double));
        if (value) {
          *(double*)value = (double)strtod(str_value, nullptr);
        };
        break;
      case OPT_TYPE_STRING:
        value = strdup(str_value);
        break;
      case OPT_TYPE_TIMEVAL:
        value = esp_malloc(sizeof(uint16_t));
        if (value) {
          *(uint16_t*)value = string2time(str_value);
        };
        break;
      case OPT_TYPE_TIMESPAN:
        value = esp_malloc(sizeof(uint32_t));
        if (value) {
          *(timespan_t*)value = string2timespan(str_value);
        };
        break;
      default:
        return nullptr;
    };
  };
  return value;
}

void* clone2value(const param_type_t type_value, void *value)
{
  void* value2 = nullptr;
  if (value) {
    switch (type_value) {
      case OPT_TYPE_I8:
        value2 = esp_malloc(sizeof(int8_t));
        if (value) {
          *(int8_t*)value2 = *(int8_t*)value;
        };
        break;
      case OPT_TYPE_U8:
        value2 = esp_malloc(sizeof(uint8_t));
        if (value) {
          *(uint8_t*)value2 = *(uint8_t*)value;
        };
        break;
      case OPT_TYPE_I16:
        value2 = esp_malloc(sizeof(int16_t));
        if (value) {
          *(int16_t*)value2 = *(int16_t*)value;
        };
        break;
      case OPT_TYPE_U16:
        value2 = esp_malloc(sizeof(uint16_t));
        if (value) {
          *(uint16_t*)value2 = *(uint16_t*)value;
        };
        break;
      case OPT_TYPE_I32:
        value2 = esp_malloc(sizeof(int32_t));
        if (value) {
          *(int32_t*)value2 = *(int32_t*)value;
        };
        break;
      case OPT_TYPE_U32:
        value2 = esp_malloc(sizeof(uint32_t));
        if (value) {
          *(uint32_t*)value2 = *(uint32_t*)value;
        };
        break;
      case OPT_TYPE_I64:
        value2 = esp_malloc(sizeof(int64_t));
        if (value) {
          *(int64_t*)value2 = *(int64_t*)value;
        };
        break;
      case OPT_TYPE_U64:
        value2 = esp_malloc(sizeof(uint64_t));
        if (value) {
          *(uint64_t*)value2 = *(uint64_t*)value;
        };
        break;
      case OPT_TYPE_FLOAT:
        value2 = esp_malloc(sizeof(float));
        if (value) {
          *(float*)value2 = *(float*)value;
        };
        break;
      case OPT_TYPE_DOUBLE:
        value2 = esp_malloc(sizeof(double));
        if (value) {
          *(double*)value2 = *(double*)value;
        };
        break;
      case OPT_TYPE_STRING:
        value2 = strdup((char*)value);
        break;
      case OPT_TYPE_TIMEVAL:
        value2 = esp_malloc(sizeof(uint16_t));
        if (value) {
          *(uint16_t*)value2 = *(uint16_t*)value;
        };
        break;
      case OPT_TYPE_TIMESPAN:
        value2 = esp_malloc(sizeof(timespan_t));
        if (value) {
          *(timespan_t*)value2 = *(timespan_t*)value;
        };
        break;
      default:
        break;
    };
  };
  return value2;
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
      case OPT_TYPE_TIMEVAL:
        return *(uint16_t*)value1 == *(uint16_t*)value2;
      case OPT_TYPE_TIMESPAN:
        return *(timespan_t*)value1 == *(timespan_t*)value2;
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

bool valueCheckLimits(const param_type_t type_value, void *value, void *value_min, void *value_max)
{
  if (value) {
    switch (type_value) {
      case OPT_TYPE_I8:
        return (!(value_min) || (*(int8_t*)value >= *(int8_t*)value_min)) && (!(value_max) || (*(int8_t*)value <= *(int8_t*)value_max));
      case OPT_TYPE_U8:
        return (!(value_min) || (*(uint8_t*)value >= *(uint8_t*)value_min)) && (!(value_max) || (*(uint8_t*)value <= *(uint8_t*)value_max));
      case OPT_TYPE_I16:
        return (!(value_min) || (*(int16_t*)value >= *(int16_t*)value_min)) && (!(value_max) || (*(int16_t*)value <= *(int16_t*)value_max));
      case OPT_TYPE_U16:
        return (!(value_min) || (*(uint16_t*)value >= *(uint16_t*)value_min)) && (!(value_max) || (*(uint16_t*)value <= *(uint16_t*)value_max));
      case OPT_TYPE_I32:
        return (!(value_min) || (*(int32_t*)value >= *(int32_t*)value_min)) && (!(value_max) || (*(int32_t*)value <= *(int32_t*)value_max));
      case OPT_TYPE_U32:
        return (!(value_min) || (*(uint32_t*)value >= *(uint32_t*)value_min)) && (!(value_max) || (*(uint32_t*)value <= *(uint32_t*)value_max));
      case OPT_TYPE_I64:
        return (!(value_min) || (*(int64_t*)value >= *(int64_t*)value_min)) && (!(value_max) || (*(int64_t*)value <= *(int64_t*)value_max));
      case OPT_TYPE_U64:
        return (!(value_min) || (*(uint64_t*)value >= *(uint64_t*)value_min)) && (!(value_max) || (*(uint64_t*)value <= *(uint64_t*)value_max));
      case OPT_TYPE_FLOAT:
        return (!(value_min) || (*(float*)value >= *(float*)value_min)) && (!(value_max) || (*(float*)value <= *(float*)value_max));
      case OPT_TYPE_DOUBLE:
        return (!(value_min) || (*(double*)value >= *(double*)value_min)) && (!(value_max) || (*(double*)value <= *(double*)value_max));
      default:
        return true;
    };
  };
  return false;
}

void setNewValue(const param_type_t type_value, void *value1, void *value2)
{
  if ((value1) && (value2)) {
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
      case OPT_TYPE_TIMEVAL:
        *(uint16_t*)value1 = *(uint16_t*)value2;
        return;
      case OPT_TYPE_TIMESPAN:
        *(timespan_t*)value1 = *(timespan_t*)value2;
        return;
      default:
        return;
    };
  };
}

static bool _nvsInit = false;

bool nvsInit()
{
  if (!_nvsInit) {
    esp_err_t err = nvs_flash_init();
    if ((err == ESP_ERR_NVS_NO_FREE_PAGES) || (err == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
      rlog_i(logTAG, "Erasing NVS partition...");
      nvs_flash_erase();
      err = nvs_flash_init();
    };
    if (err == ESP_OK) {
      _nvsInit = true;
      rlog_i(logTAG, "NVS partition initilized");
    }
    else {
      rlog_e(logTAG, "NVS partition initialization error: %d (%s)", err, esp_err_to_name(err));
    };
  };
  return _nvsInit;
}

bool nvsOpen(const char* name_group, nvs_open_mode_t open_mode, nvs_handle_t *nvs_handle)
{
  esp_err_t err = nvs_open(name_group, open_mode, nvs_handle); 
  if (err != ESP_OK) {
    if (!((err == ESP_ERR_NVS_NOT_FOUND) && (open_mode == NVS_READONLY))) {
      rlog_e(logTAG, "Error opening NVS namespace \"%s\": %d (%s)!", name_group, err, esp_err_to_name(err));
    };
    return false;
  };
  return true;
}

bool nvsRead(const char* name_group, const char* name_key, const param_type_t type_value, void * value)
{
  // Check values
  if (!name_key) {
    rlog_e(logTAG, "Failed to read value: name_key is NULL!");
    return false;
  };
  if (!value) {
    rlog_e(logTAG, "Failed to read NULL value!");
    return false;
  };

  nvs_handle_t nvs_handle;
  // Open NVS namespace
  if (!nvsOpen(name_group, NVS_READONLY, &nvs_handle)) return false;

  // Read value
  esp_err_t err = ESP_OK;
  if (type_value == OPT_TYPE_STRING) {
    // Get the size of the string that is in the storage
    size_t new_len = 0;
    err = nvs_get_str(nvs_handle, name_key, nullptr, &new_len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
      rlog_d(logTAG, "Value \"%s.%s\" is not initialized yet, used default: [%s]", name_group, name_key, (char*)value);
    }
    else {
      if (err != ESP_OK) {
        rlog_w(logTAG, "Error reading string \"%s.%s\": %d (%s)!", name_group, name_key, err, esp_err_to_name(err));
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
        value = esp_malloc(new_len);
      };
      // Reading a line from storage
      err = nvs_get_str(nvs_handle, name_key, (char*)value, &new_len);
      if (err == ESP_OK) {
        // It's okay, delete the previous value
        if (prev_value) {
          free(prev_value);
        };
        rlog_d(logTAG, "Read string value \"%s.%s\": [%s]", name_group, name_key, (char*)value);
      } else {
        // We delete the allocated memory for new data and return the previous value
        if (prev_value) {
          free(value);
          value = prev_value;
          prev_value = nullptr;
        };
        rlog_e(logTAG, "Error reading string \"%s.%s\": %d (%s)!", name_group, name_key, err, esp_err_to_name(err));
      };
    };
  } else {
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
        err = nvs_get_float(nvs_handle, name_key, (float*)value);
        break;
      case OPT_TYPE_DOUBLE:
        err = nvs_get_double(nvs_handle, name_key, (double*)value);
        break;
      case OPT_TYPE_TIMEVAL:
        err = nvs_get_u16(nvs_handle, name_key, (uint16_t*)value);
        break;
      case OPT_TYPE_TIMESPAN:
        err = nvs_get_u32(nvs_handle, name_key, (uint32_t*)value);
        break;
      default:
        err = ESP_ERR_NVS_TYPE_MISMATCH;
        break;
    };
    
    #if CONFIG_RLOG_PROJECT_LEVEL >= RLOG_LEVEL_ERROR
      if (name_group && name_key) {
        char* str_value = value2string(type_value, value);
        RE_MEM_CHECK(str_value, return true);
        switch (err) {
          case ESP_OK:
            rlog_d(logTAG, "Read value \"%s.%s\": [%s]", name_group, name_key, str_value);
            break;
          case ESP_ERR_NVS_NOT_FOUND:
            rlog_d(logTAG, "Value \"%s.%s\" is not initialized yet, used default: [%s]", name_group, name_key, str_value);
            break;
          default :
            rlog_e(logTAG, "Error reading \"%s.%s\": %d (%s)!", name_group, name_key, err, esp_err_to_name(err));
            break;
        };
        free(str_value);
      };
    #endif // CONFIG_RLOG_PROJECT_LEVEL
  };

  nvs_close(nvs_handle);
  return (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND);
}

bool nvsWrite(const char* name_group, const char* name_key, const param_type_t type_value, void * value)
{
  // Check values
  if (!name_key) {
    rlog_e(logTAG, "Failed to write value: name_key is NULL!");
    return false;
  };
  if (!value) {
    rlog_e(logTAG, "Failed to write NULL value!");
    return false;
  };

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
      err = nvs_set_float(nvs_handle, name_key, *(float*)value);
      break;
    case OPT_TYPE_DOUBLE:
      err = nvs_set_double(nvs_handle, name_key, *(double*)value);
      break;
    case OPT_TYPE_STRING:
      err = nvs_set_str(nvs_handle, name_key, (char*)value);
      break;
    case OPT_TYPE_TIMEVAL:
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

  #if CONFIG_RLOG_PROJECT_LEVEL >= RLOG_LEVEL_ERROR
    if (name_group && name_key) {
      if (err == ESP_OK) {
        rlog_i(logTAG, "Value \"%s.%s\" was successfully written to storage", name_group, name_key);
      }
      else {
        rlog_e(logTAG, "Error writting \"%s.%s\": %d (%s)!", name_group, name_key, err, esp_err_to_name(err));
      };
    };
  #endif // CONFIG_RLOG_PROJECT_LEVEL

  nvs_close(nvs_handle);
  return (err == ESP_OK);
}


