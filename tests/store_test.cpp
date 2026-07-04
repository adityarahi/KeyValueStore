#include <gtest/gtest.h>

#include "KVStore.h"

class KVStoreTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Code here runs immediately before each test
    // cleanup is not needed since
    // TEST_F creates a fresh KVStore for each test automatically
    // if(!obj.empty()) obj.cleanUp();
  }

  void TearDown() override {
    // Code here runs immediately after each test
    // obj.cleanUp();
  }

  KVStore obj;
};

TEST_F(KVStoreTest, SetAndGet) {
  obj.set("25", "50");
  EXPECT_EQ(obj.get("25"), "50");  // Non-fatal assertion
}

TEST_F(KVStoreTest, GetMissingKey) {
  obj.set("25", "50");
  EXPECT_EQ(obj.get("7"), std::nullopt);  // nullopt check
}

TEST_F(KVStoreTest, DelExistingKey) {
  obj.set("25", "50");
  obj.set("15", "30");
  ASSERT_TRUE(obj.del("25"));  // Fatal assertion
}

TEST_F(KVStoreTest, DelMissingKey) {
  obj.set("25", "50");
  ASSERT_FALSE(obj.del("15"));
}

TEST_F(KVStoreTest, OverwriteValue) {
  obj.set("25", "50");
  EXPECT_EQ(obj.get("25"), "50");  // Non-fatal assertion
  obj.set("25", "75");
  ASSERT_TRUE(obj.get("25") == "75");  // Fatal assertion
}