#include <gtest/gtest.h>

#include "store/KVStoreSharded.h"
#include "net/RespParser.h"

class RespParserTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}

  RespParser obj;
};


TEST_F(RespParserTest, ParseSetCommand) {
  std::string raw = "*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n";
  std::vector<std::string> parsed = {"SET", "foo", "bar"};
  EXPECT_EQ(obj.parse(raw), parsed);  // Non-fatal assertion
}

TEST_F(RespParserTest, ParseGetCommand) {
  std::string raw = "*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n";
  std::vector<std::string> parsed = {"GET", "foo"};
  EXPECT_EQ(obj.parse(raw), parsed);  // Non-fatal assertion
}

TEST_F(RespParserTest, ParseDelCommand) {
  std::string raw = "*2\r\n$3\r\nDEL\r\n$3\r\nfoo\r\n";
  std::vector<std::string> parsed = {"DEL", "foo"};
  EXPECT_EQ(obj.parse(raw), parsed);  // Non-fatal assertion
}

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

  KVStoreSharded obj;
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

TEST_F(KVStoreTest, ExpireKey) {
  obj.set("25", "50");
  EXPECT_EQ(obj.get("25"), "50");  // Non-fatal assertion
  obj.expire("25", 1);
  sleep(2);
  ASSERT_TRUE(obj.get("25") == std::nullopt);  // Fatal assertion
}

TEST_F(KVStoreTest, LRUEviction) {
  for(int i = 1; i <= 16001; i++) {
    obj.set("key_" + std::to_string(i), "value_" + std::to_string(i));
  }
  ASSERT_TRUE(obj.size() < 16001);
}