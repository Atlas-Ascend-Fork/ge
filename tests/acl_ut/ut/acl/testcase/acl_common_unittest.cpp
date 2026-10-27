#include <gtest/gtest.h>
#include <vector>
#include "common/common_inner.h"
#include "common/log_inner.h"
#include "common/acl_json_parser.h"
#include "common/prof_api_reg.h"
#include "acl_stub.h"

using namespace testing;
using namespace std;
using namespace acl;

std::vector<ProfCommandHandle> &GetProfCtrlHandles();

class UTEST_ACL_Common : public testing::Test {
 protected:
  virtual void SetUp() {
    MockFunctionTest::aclStubInstance().ResetToDefaultMock();
  }
  virtual void TearDown() {
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
  }
};

TEST_F(UTEST_ACL_Common, GetCurLogLevel1) {
  EXPECT_CALL(MockFunctionTest::aclStubInstance(), dlog_getlevel(_, _)).WillOnce(Return((DLOG_ERROR)));
  uint32_t log_level = AclLog::GetCurLogLevel();
  EXPECT_EQ(log_level, ACL_ERROR);
}

TEST_F(UTEST_ACL_Common, GetCurLogLevel2) {
  EXPECT_CALL(MockFunctionTest::aclStubInstance(), dlog_getlevel(_, _)).WillOnce(Return((DLOG_WARN)));
  uint32_t log_level = AclLog::GetCurLogLevel();
  EXPECT_EQ(log_level, ACL_WARNING);
}

TEST_F(UTEST_ACL_Common, GetCurLogLevel3) {
  EXPECT_CALL(MockFunctionTest::aclStubInstance(), dlog_getlevel(_, _)).WillOnce(Return((DLOG_INFO)));
  uint32_t log_level = AclLog::GetCurLogLevel();
  EXPECT_EQ(log_level, ACL_INFO);
}

TEST_F(UTEST_ACL_Common, GetCurLogLevel4) {
  EXPECT_CALL(MockFunctionTest::aclStubInstance(), dlog_getlevel(_, _)).WillOnce(Return((DLOG_DEBUG)));
  uint32_t log_level = AclLog::GetCurLogLevel();
  EXPECT_EQ(log_level, ACL_DEBUG);
}

TEST_F(UTEST_ACL_Common, GetCurLogLevel5) {
  EXPECT_CALL(MockFunctionTest::aclStubInstance(), dlog_getlevel(_, _)).WillOnce(Return((DLOG_NULL)));
  uint32_t log_level = AclLog::GetCurLogLevel();
  EXPECT_EQ(log_level, ACL_INFO);
}

extern int g_logLevel;
TEST_F(UTEST_ACL_Common, ACLSaveLog) {
  const char *strLog = "hello, acl";
  AclLog::ACLSaveLog(ACL_DEBUG, strLog);
  EXPECT_EQ(g_logLevel, ACL_DEBUG);
  AclLog::ACLSaveLog(ACL_INFO, strLog);
  EXPECT_EQ(g_logLevel, ACL_INFO);
  AclLog::ACLSaveLog(ACL_WARNING, strLog);
  EXPECT_EQ(g_logLevel, ACL_WARNING);
  AclLog::ACLSaveLog(ACL_ERROR, strLog);
  EXPECT_EQ(g_logLevel, ACL_ERROR);
  AclLog::ACLSaveLog(ACL_ERROR, nullptr);
}

TEST_F(UTEST_ACL_Common, jsonParserUtilsTest) {
  nlohmann::json js;
  auto ret = JsonParser::ParseJson("{", js);
  EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
  ret = JsonParser::ParseJson(nullptr, js);
  EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
  ret = JsonParser::ParseJson("{}", js);
  EXPECT_EQ(ret, ACL_RT_SUCCESS);
  std::string strJsonCtx;
  std::string subStrKey = "key";
  bool found = false;
  ret = JsonParser::GetJsonCtxByKeyFromBuffer("{", strJsonCtx, subStrKey, found);
  EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
  ret = JsonParser::GetJsonCtxByKeyFromBuffer(nullptr, strJsonCtx, subStrKey, found);
  EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
  EXPECT_EQ(found, false);
  ret = JsonParser::GetJsonCtxByKeyFromBuffer("{\"key\":\"value\"}", strJsonCtx, subStrKey, found);
  EXPECT_EQ(ret, ACL_RT_SUCCESS);
  EXPECT_EQ(found, true);
  EXPECT_EQ(strJsonCtx, "\"value\"");
  ret = JsonParser::GetJsonCtxByKeyFromBuffer("{\"key1\":\"value\"}", strJsonCtx, subStrKey, found);
  EXPECT_EQ(ret, ACL_RT_SUCCESS);
  EXPECT_EQ(found, false);
}

TEST_F(UTEST_ACL_Common, GetJsonCtxByKey_failed) {
  nlohmann::json js;
  const char_t *fileName = ACL_BASE_DIR "/tests/ut/acl/json/illegal_format.json";
  std::string strJsonCtx = "123";
  std::string subStrKey;
  bool found = false;
  aclError ret = acl::JsonParser::GetJsonCtxByKeyFromBuffer(fileName, strJsonCtx, subStrKey, found);
  EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
  EXPECT_FALSE(found);
}

TEST_F(UTEST_ACL_Common, AclProfilingReporter) {
  acl::AclProfilingReporter reporter(acl::AclProfType::AclmdlSetDynamicHWSize);
  EXPECT_EQ(reporter.aclApi_, acl::AclProfType::AclmdlSetDynamicHWSize);
  reporter.~AclProfilingReporter();
  acl::AclProfilingReporter reporter1(acl::AclProfType::AclmdlExecute);
  EXPECT_EQ(reporter1.aclApi_, acl::AclProfType::AclmdlExecute);
  reporter1.~AclProfilingReporter();
}

namespace {
constexpr uint64_t UT_PROF_SWITCH_ACL_API = 0x0001U;
constexpr uint32_t UT_START_PROFILING = 1U;
constexpr uint32_t UT_STOP_PROFILING = 2U;

MsprofCommandHandle MakeProfSwitchConfig(uint32_t type) {
  MsprofCommandHandle config{};
  config.profSwitch = UT_PROF_SWITCH_ACL_API;
  config.type = type;
  config.devNums = 1U;
  config.devIdList[0] = 0U;
  return config;
}
}  // namespace

TEST_F(UTEST_ACL_Common, ProfInnerStartRegisterTypeFailed) {
  const auto &handles = GetProfCtrlHandles();
  ASSERT_FALSE(handles.empty());
  EXPECT_CALL(MockFunctionTest::aclStubInstance(), MsprofRegTypeInfo(_, _, _))
      .WillOnce(Return(MSPROF_ERROR_NONE))
      .WillRepeatedly(Return(-1));
  auto config = MakeProfSwitchConfig(UT_START_PROFILING);
  for (const auto &handle : handles) {
    EXPECT_EQ(handle(PROF_CTRL_SWITCH, &config, sizeof(MsprofCommandHandle)), ACL_ERROR_PROFILING_FAILURE);
  }
  EXPECT_CALL(MockFunctionTest::aclStubInstance(), MsprofSysCycleTime()).Times(0);
  EXPECT_CALL(MockFunctionTest::aclStubInstance(), MsprofReportApi(_, _)).Times(0);
  {
    acl::AclProfilingReporter reporter(acl::AclProfType::AclopExecute);
  }
}

TEST_F(UTEST_ACL_Common, ProfInnerStartSuccessReportAndStop) {
  const auto &handles = GetProfCtrlHandles();
  ASSERT_FALSE(handles.empty());
  EXPECT_CALL(MockFunctionTest::aclStubInstance(), MsprofRegTypeInfo(_, _, _))
      .WillRepeatedly(Return(MSPROF_ERROR_NONE));
  auto startConfig = MakeProfSwitchConfig(UT_START_PROFILING);
  for (const auto &handle : handles) {
    EXPECT_EQ(handle(PROF_CTRL_SWITCH, &startConfig, sizeof(MsprofCommandHandle)), ACL_SUCCESS);
  }
  EXPECT_CALL(MockFunctionTest::aclStubInstance(), MsprofSysCycleTime()).WillRepeatedly(Return(1UL));
  EXPECT_CALL(MockFunctionTest::aclStubInstance(), MsprofReportApi(_, _)).Times(1).WillOnce(Return(MSPROF_ERROR_NONE));
  {
    acl::AclProfilingReporter reporter(acl::AclProfType::AclopExecute);
  }
  auto stopConfig = MakeProfSwitchConfig(UT_STOP_PROFILING);
  for (const auto &handle : handles) {
    EXPECT_EQ(handle(PROF_CTRL_SWITCH, &stopConfig, sizeof(MsprofCommandHandle)), ACL_SUCCESS);
  }
}

TEST_F(UTEST_ACL_Common, FormatStr_failed_1100bytes) {
  std::string str(1100, 'a');
  std::string value = acl::AclErrorLogManager::FormatStr(str.c_str());
  EXPECT_TRUE(value.empty());

  value = acl::AclErrorLogManager::FormatStr(nullptr);
  EXPECT_TRUE(value.empty());
}
