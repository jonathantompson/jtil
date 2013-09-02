//
//  settings_manager.h
//
//  Created by Jonathan Tompson on 5/11/12.
//
//  Stores and manages a list of settings, can only load from ONE file, but
//  you can save to as many as you want.

#pragma once

#ifdef __APPLE__
  #include <unordered_map>
#endif
#include <string>
#ifdef _WIN32
#include <hash_map>
#endif
#include "jtil/settings/setting.h"
#include "jtil/data_str/vector_managed.h"
#include "jtil/data_str/hash_map_managed.h"
#include "jtil/data_str/Pair.h"  // Needed for GET_SETTING and SET_SETTING macros
#include "jtil/math/math_types.h"  // For MAX_UINT32
#include "jtil/data_str/hash_funcs.h"
#include "jtil/string_util/macros.h"  // For INLINE

#define SM_TEMPFILE "temp_SettingsManager_temp.csv"
// #define UI_COMBOBOX_NUM_TOKENS 12
// #define UI_RUNTIMELABEL_NUM_TOKENS 7

namespace jtil {
namespace settings {

  class SettingsManager {
  public:
    static void initSettings(const std::string& filename);
    static void saveSettings(const std::string& filename);
    static void shutdownSettingsManager();

    // Add a new ui element to the global array
    static void addSetting(SettingBase* obj);

    // Just get the settingbase matching the ID - O(1):
    static SettingBase* getSetting(const std::string& key);
    static SettingBase* getSettingPrehash(const uint32_t prehash,
      const std::string& key);
    
    // Get and set the setting when the type is known - O(1)
    template <class T> static bool getSetting(const std::string& key, T& val);
    template <class T> static bool getSettingPtr(const std::string& key,
      T*& val);
    template <class T> static bool setSetting(const std::string& key, 
      const T& val);
    template <class T> static bool getSettingPrehash(const uint32_t hash, 
      const std::string& key, T& val);
    template <class T> static bool getSettingPtrPrehash(const uint32_t hash,
      const std::string& key, T*& val);
    template <class T> static bool setSettingPrehash(const uint32_t hash, 
      const std::string& key, const T& val);

    static data_str::HashMapManaged<std::string, SettingBase*>* 
      g_setting_arr() { return g_setting_arr_; }

  private:
    SettingsManager();  // Use init, save and shutdown functions!
    ~SettingsManager();

    static std::string filename_;
    static data_str::HashMapManaged<std::string, SettingBase*>* g_setting_arr_;

    static void parseToken(data_str::VectorManaged<const char*>& cur_token, 
      const std::string& filename);    

    static void loadSettingsFromFile(const std::string& filename);
    static void saveSettingsToFile(const std::string& old_file, 
      const std::string& new_file);

    // Non-copyable, non-assignable.
    SettingsManager(SettingsManager&);
    SettingsManager& operator=(const SettingsManager&);
  };
  
  template <class T> 
  INLINE bool SettingsManager::getSetting(const std::string& key, T& val) {
    SettingBase* elem = getSetting(key);
    if (elem != NULL) {
      Setting<T>* elem_set = static_cast<Setting<T>*>(elem);
      if (elem_set != NULL) {
        val = elem_set->val();
        return true;
      }
    }
    return false;  // If we've gotton to here, the setting doesn't exist
  };

  template <class T> 
  INLINE bool SettingsManager::getSettingPtr(const std::string& key, T*& val) {
    SettingBase* elem = getSetting(key);
    if (elem != NULL) {
      Setting<T>* elem_set = static_cast<Setting<T>*>(elem);
      if (elem_set != NULL) {
        val = elem_set->val_ptr();
        return true;
      }
    }
    return false;  // If we've gotton to here, the setting doesn't exist
  };
  
  template <class T> 
  INLINE bool SettingsManager::setSetting(const std::string& key, 
    const T& val) {
    SettingBase* elem = getSetting(key);
    if (elem != NULL) {
      Setting<T>* elem_set = static_cast<Setting<T>*>(elem);
      if (elem_set != NULL) {
        elem_set->val(val);
        return true;
      }
    }
    return false;  // If we've gotton to here, the setting doesn't exist
  };

  template <class T> 
  INLINE bool SettingsManager::getSettingPrehash(const uint32_t prehash,
    const std::string& key, T& val) {
    SettingBase* elem = getSettingPrehash(prehash, key);
    if (elem != NULL) {
      Setting<T>* elem_set = static_cast<Setting<T>*>(elem);
      if (elem_set != NULL) {
        val = elem_set->val();
        return true;
      }
    }
    return false;  // If we've gotton to here, the setting doesn't exist
  };

  template <class T> 
  INLINE bool SettingsManager::getSettingPtrPrehash(const uint32_t prehash,
    const std::string& key, T*& val) {
    SettingBase* elem = getSettingPrehash(prehash, key);
    if (elem != NULL) {
      Setting<T>* elem_set = static_cast<Setting<T>*>(elem);
      if (elem_set != NULL) {
        val = elem_set->val_ptr();
        return true;
      }
    }
    return false;  // If we've gotton to here, the setting doesn't exist
  };
  
  template <class T> 
  INLINE bool SettingsManager::setSettingPrehash(const uint32_t prehash,
    const std::string& key, const T& val) {
    SettingBase* elem = getSettingPrehash(prehash, key);
    if (elem != NULL) {
      Setting<T>* elem_set = static_cast<Setting<T>*>(elem);
      if (elem_set != NULL) {
        elem_set->val(val);
        return true;
      }
    }
    return false;  // If we've gotton to here, the setting doesn't exist
  };

#ifdef _WIN32
  // getSetting - Find the SettingsManagerElem in the vector from the given 
  // var + id pair instance 
  // NOTE: ANY CHANGES MADE HERE MUST ALSO BE MADE IN THE __APPLE__ VERSION
  //       IN settings_manager.cpp
  INLINE SettingBase* SettingsManager::getSetting(const std::string& key) { 
    SettingBase* ret;
    if (g_setting_arr_->lookup(key, ret)) {
      return ret;
    } else {
      return NULL;
    }
  }
  INLINE SettingBase* SettingsManager::getSettingPrehash(
    const uint32_t prehash, const std::string& key) { 
    SettingBase* ret;
    if (g_setting_arr_->lookupPrehash(prehash, key, ret)) {
      return ret;
    } else {
      return NULL;
    }
  }
#endif

};  // namespace settings
};  // namespace jtil

// Debug version keeps around key as well for error messages, also the hash 
// value is statically generated at compile time for speed in the release build
#if defined(_DEBUG) || defined(DEBUG)
  #define SET_SETTING(name, type, val) \
  if (!jtil::settings::SettingsManager::setSetting<type>(name, val)) { \
    std::stringstream st; \
    st << "SET_SETTING ERROR: Line("; \
    st << __LINE__; \
    st << "), File(" << __FILE__ << ")\n"; \
    st << "                   Cannot find setting named: " << name; \
    throw std::wruntime_error(st.str()); \
  }
  #define GET_SETTING(name, type, ret) \
  if (!jtil::settings::SettingsManager::getSetting<type>(name, ret)) { \
    std::stringstream st; \
    st << "GET_SETTING ERROR: Line("; \
    st << __LINE__; \
    st << "), File(" << __FILE__ << ")\n"; \
    st << "                   Cannot find setting named: " << name; \
    throw std::wruntime_error(st.str()); \
  }
  #define GET_SETTING_PTR(name, type, ret) \
  if (!jtil::settings::SettingsManager::getSettingPtr<type>(name, ret)) { \
    std::stringstream st; \
    st << "GET_SETTING ERROR: Line("; \
    st << __LINE__; \
    st << "), File(" << __FILE__ << ")\n"; \
    st << "                   Cannot find setting named: " << name; \
    throw std::wruntime_error(st.str()); \
  }
#else
  #define SET_SETTING(name, type, val) \
  if (!jtil::settings::SettingsManager::setSettingPrehash<type>(CONSTANT_HASH( \
    jtil::settings::SettingsManager::g_setting_arr()->size(), name), name, val)) { \
    throw std::wruntime_error("SET_SETTING ERROR: Cannot find setting."); \
  }
  #define GET_SETTING(name, type, ret) \
  if (!jtil::settings::SettingsManager::getSettingPrehash<type>(CONSTANT_HASH( \
    jtil::settings::SettingsManager::g_setting_arr()->size(), name), name, ret)) { \
    throw std::wruntime_error("GET_SETTING ERROR: Cannot find setting."); \
  }
  #define GET_SETTING_PTR(name, type, ret) \
  if (!jtil::settings::SettingsManager::getSettingPtr<type>(CONSTANT_HASH( \
    jtil::settings::SettingsManager::g_setting_arr()->size(), name), name, ret)) { \
    throw std::wruntime_error("GET_SETTING_PTR ERROR: Cannot find setting."); \
  }
#endif
