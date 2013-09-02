#include <sys/stat.h>
#include <string>
#include "jtil/settings/settings_manager.h"
#include "jtil/data_str/vector_managed.h"
#include "jtil/data_str/pair.h"
#include "jtil/data_str/hash_map_managed.h"
#include "jtil/data_str/hash_funcs.h"
#include "jtil/file_io/csv_handle_read.h"
#include "jtil/file_io/csv_handle_write.h"
#include "jtil/string_util/macros.h"  // For INLINE
#include "jtil/exceptions/wruntime_error.h"

#ifndef NULL
#define NULL 0
#endif
#define STARTING_HASHMAPMANAGED_SIZE 271  // Big prime to avoid collisions
#define SAFE_DELETE(x) if (x != NULL) { delete x; x = NULL; }

using std::wruntime_error;
using std::string;

namespace jtil {

using data_str::HashMapManaged;
using data_str::VectorManaged;
using data_str::Pair;
using file_io::CSVHandleRead;
using file_io::CSVHandleWrite;
using file_io::CSVHandle;

namespace settings {

  HashMapManaged<string, SettingBase*>* SettingsManager::g_setting_arr_ = NULL;
  string SettingsManager::filename_;

  void SettingsManager::shutdownSettingsManager() {
    SAFE_DELETE(g_setting_arr_);
  }

  bool FileExists(const char *filename) {
    struct stat   buffer;   
    return (stat (filename, &buffer) == 0);
  }

  void SettingsManager::initSettings(const string& filename) {
    // Make sure the file exists (try opening it)
    std::ifstream file;
    file.open(filename.c_str(), std::ios::in);
    if (!file.is_open()) {
      std::stringstream ss;
      ss << "jtil::SettingsManager::initSettings() - ERROR: settings file ";
      ss << filename << " does not exist.  Are you sure you copied over ";
      ss << "settings.csv to the run directory?";
      throw wruntime_error(ss.str());
    }
    file.close();

    if (g_setting_arr_ != NULL) {  //  Should only call init once!
      throw wruntime_error(L"SettingsManager::initSettings() - Init"
        L" called twice!");
    }

    filename_ = filename;
    g_setting_arr_ = new HashMapManaged<std::string, SettingBase*>(
      STARTING_HASHMAPMANAGED_SIZE, &data_str::HashString);
    loadSettingsFromFile(filename);
  }

  void SettingsManager::saveSettings(const string& filename) {
    if (g_setting_arr_ == NULL) {
      return;
    }

    // Save the settings to a temporary file
    saveSettingsToFile(filename_, string(SM_TEMPFILE));

    if (FileExists(filename.c_str())) {
      if (remove(filename.c_str()) != 0) {
        throw wruntime_error(L"SettingsManager::saveSettings() - Error"
          L" deleting old settings file");
      }
    }

    if (rename(SM_TEMPFILE, filename.c_str()) != 0) {
      throw wruntime_error(L"SettingsManager::saveSettings() - Error"
        L" renaming new settings file");
    }
  }

  // LoadSettingsManagerFromFile - Read in all the SettingsManagerElem from 
  // file: settings, HWIN etc...
  void SettingsManager::loadSettingsFromFile(const string& filename) { 
    CSVHandleRead reader(filename);

    VectorManaged<const char*> cur_token;  // Each element is a csv in the line
    static const bool inc_whitespace = false;

    // Keep reading tokens until we're at the end
    while (!reader.checkEOF()) {
      reader.readNextToken(cur_token, inc_whitespace);  // Get the next elem
      if (cur_token.size() > 0)
        SettingsManager::parseToken(cur_token, filename);  // process object

      cur_token.clear();  // Clear the token
    }

    reader.close();
  }

  // ProcessToken - Make a new object for every token.
  void SettingsManager::parseToken(VectorManaged<const char*>& cur_token, 
    const string& filename) {
    std::string elementHeader(cur_token[0]);
    SettingBase* new_setting = NULL;

    // Switch statement here would be ideal, but using wchar_t * in switch is 
    // messy (must be const case values)
    if (elementHeader == "//") {
      // Do nothing for comment blocks
    } else if (elementHeader == "") {
      // Do nothing for empty lines
    } else {
      std::string key(cur_token[1]);
      new_setting = SettingBase::parseToken(cur_token, filename);
      SettingsManager::addSetting(new_setting);  // Add to get unique ID
    } 
  }

  // addSetting - Add the obj to the vector array
  void SettingsManager::addSetting(SettingBase* obj) { 
    // Make sure there isn't another element with the same name
    SettingBase* tmp;
    if (g_setting_arr_->lookup(obj->name(), tmp)) {
      if (tmp->name() == obj->name()) {
        string error = string("SettingsManager::addSetting() - setting name") + 
          string(" already exists (names must be unique): ") + obj->name(); 
        throw wruntime_error(error);
      } else {
        string error = string("SettingsManager::addSetting() - setting ID") + 
          string(" already exists (strings hash to the same ID!): ") + 
          obj->name() + string(", ") + tmp->name();
        throw wruntime_error(error);
      }
    }
    g_setting_arr_->insert(obj->name(), obj);
  }

  // saveSettingsToFile - Save the settings to the new .csv file without 
  // Destroying the formatting.
  void SettingsManager::saveSettingsToFile(const string& old_file, 
    const string& new_file) {
    CSVHandleRead reader(old_file);
    CSVHandleWrite writer(new_file);

    VectorManaged<const char*> cur_token;  // Each element is a csv in the line
    bool inc_whitespace = true;

    // Keep reading tokens until we're at the end
    while (!reader.checkEOF()) {
      reader.readNextToken(cur_token, inc_whitespace);  // Get the next elem
      if (cur_token.size() > 0)
        SettingBase::writeToken(cur_token, writer, old_file);
      else
        if (!reader.checkEOF())
          writer.writeLine(const_cast<char*>("\n"));
      cur_token.clear();  // Clear the token
    }

    // Close the file (it would be done anyway by the destructor)
    reader.close();
    writer.close();
  }
  
#ifdef __APPLE__
  // getSetting - Find the SettingsManagerElem in the vector from the given 
  // var + id pair instance 
  // NOTE: ANY CHANGES MADE HERE MUST ALSO BE MADE IN THE _WIN32 VERSION
  //       IN settings_manager.h
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

}  // namespace settings
}  // namespace jtil
