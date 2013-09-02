//
//  test_settings_manager.cpp
//
//  Created by Jonathan Tompson on 3/14/12.
//
//  LOTS of tests here.  There's a lot of code borrowed from Prof 
//  Alberto Lerner's code repository (I took his distributed computing class, 
//  which was amazing), particularly the test unit stuff.

#define TEST_FILE "settings_test.csv"
#define TEST_FILE_TMP "settings_test_tmp.csv"

#include <string>
#include <iostream>
#include "test_unit/test_unit.h"
#include "jtil/settings/settings_manager.h"
#include "jtil/settings/setting.h"
#include "jtil/math/math_types.h"
#include "jtil/data_str/hash_funcs.h"
#include "jtil/file_io/file_io.h"
extern "C" {
  #include "jtil/string_util/md5.h"
}

using jtil::settings::SettingsManager;
using jtil::settings::Setting;
using std::string;
using std::cout;
using std::endl;
using jtil::math::Float3;

// CRC Calculator: http://www.lammertbies.nl/comm/info/crc-calculation.html
// This is the FNV-1a
TEST(SettingsManager, CRCGeneration) {
  uint32_t hashA = DYNAMIC_HASH(4294967295u, "aVeryLongTestWhichStillWorks");
  uint32_t hashB = CONSTANT_HASH(4294967295u, "aVeryLongTestWhichStillWorks");
  EXPECT_EQ(hashA, hashB);

  uint32_t hashC = DYNAMIC_HASH(4294967295u, "@nother Stup1d3Xample");
  uint32_t hashD = CONSTANT_HASH(4294967295u, "@nother Stup1d3Xample");
  EXPECT_EQ(hashC, hashD);
}

TEST(SettingsManager, LoadingAndSetting) {
  if (jtil::file_io::fileExists(TEST_FILE)) {
    SettingsManager::initSettings(TEST_FILE);

    // Test a bool value, get the value, then set the value
    bool shadowsOn;
    GET_SETTING("shadowsOn", bool, shadowsOn);
    EXPECT_EQ(shadowsOn, true);
    SET_SETTING("shadowsOn", bool, false);
    GET_SETTING("shadowsOn", bool, shadowsOn);
    EXPECT_EQ(shadowsOn, false);
    SET_SETTING("shadowsOn", bool, true);  // Put it back to what it was

    // Test an int value, get the value, then set the value
    int collisionVectorSize;
    GET_SETTING("collisionVectorSize", int, collisionVectorSize);
    EXPECT_EQ(collisionVectorSize, 1024);
    SET_SETTING("collisionVectorSize", int, 1023);
    GET_SETTING("collisionVectorSize", int, collisionVectorSize);
    EXPECT_EQ(collisionVectorSize, 1023);
    SET_SETTING("collisionVectorSize", int, 1024);

    // Test a float value, get the value, then set the value
    float appVersionNumber;
    GET_SETTING("appVersionNumber", float, appVersionNumber);
    EXPECT_EQ(appVersionNumber, 2.0f);
    SET_SETTING("appVersionNumber", float, 3.14159f);
    GET_SETTING("appVersionNumber", float, appVersionNumber);
    EXPECT_EQ(appVersionNumber, 3.14159f);
    SET_SETTING("appVersionNumber", float, 2.0f);

    // Test a string value, get the value, then set the value
    string appName;
    GET_SETTING("appName", std::string, appName);
    EXPECT_EQ(appName, string("PRenderer2"));
    SET_SETTING("appName", string, string("silly_string"));
    GET_SETTING("appName", string, appName);
    EXPECT_EQ(appName, string("silly_string"));
    SET_SETTING("appName", string, string("PRenderer2")); 

    // Test a Float3 value, get the value, then set the value
    Float3 startingGravity;
    Float3 startingGravity_correct(0.0f, -9.8f, 0.0f);
    GET_SETTING("startingGravity", Float3, startingGravity);
    EXPECT_TRUE(Float3::equal(startingGravity, startingGravity_correct));
    Float3 startingGravity_wrong(-1, -1, -1);
    SET_SETTING("startingGravity", Float3, startingGravity_wrong);
    GET_SETTING("startingGravity", Float3, startingGravity);
    EXPECT_TRUE(Float3::equal(startingGravity, startingGravity_wrong));
    SET_SETTING("startingGravity", Float3, startingGravity_correct);

    SettingsManager::shutdownSettingsManager();
  } else {
    std::cout << "Warning, " << TEST_FILE;
    std::cout << " is not in the current run directory.";
    std::cout << "Test wont be run.";
  }
}

// It worked, I but now it will get stuck in deugger.
//TEST(SettingsManager, LoadingAndSettingNonExistant) {
//  SettingsManager::initSettings(TEST_FILE);
//
//  bool caught_exception = false;
//  try {
//    Float3 WontEverExist;
//    GET_SETTING("WontEverExist", Float3, WontEverExist);
//  } catch(std::wruntime_error e) {
//    caught_exception = true;
//  }
//  EXPECT_TRUE(caught_exception);
//
//  SettingsManager::shutdownSettingsManager();
//}

TEST(SettingsManager, SaveToFile) {
  if (jtil::file_io::fileExists(TEST_FILE)) {
    SettingsManager::initSettings(TEST_FILE);

    unsigned char md5_settings[16];
    MD5File(TEST_FILE, md5_settings);

    SettingsManager::saveSettings(TEST_FILE_TMP);
    unsigned char md5_settings_test[16];
    MD5File(TEST_FILE_TMP, md5_settings_test);
    if (remove( TEST_FILE_TMP ) != 0)
      throw std::wruntime_error("Error deleting temp settings file");

    // Now make sure that the MD5 checksums are the same
    for (int i = 0; i < 16; i ++) {
      EXPECT_EQ(md5_settings[i], md5_settings_test[i]);
    }

    SettingsManager::shutdownSettingsManager();
  } else {
    std::cout << "Warning, " << TEST_FILE;
    std::cout << " is not in the current run directory.";
    std::cout << "Test wont be run.";
  }
}
