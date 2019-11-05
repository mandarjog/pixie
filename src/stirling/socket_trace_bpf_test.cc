#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include <string_view>
#include <thread>

#include "src/common/system/testing/tcp_socket.h"
#include "src/shared/metadata/metadata.h"
#include "src/shared/types/column_wrapper.h"
#include "src/shared/types/types.h"
#include "src/stirling/bcc_bpf_interface/socket_trace.h"
#include "src/stirling/data_table.h"
#include "src/stirling/mysql/test_data.h"
#include "src/stirling/mysql/test_utils.h"
#include "src/stirling/socket_trace_connector.h"
#include "src/stirling/testing/client_server_system.h"

namespace pl {
namespace stirling {

using ::pl::stirling::testing::TCPSocket;
using ::pl::types::ColumnWrapper;
using ::pl::types::ColumnWrapperRecordBatch;
using ::testing::HasSubstr;
using ::testing::Pair;
using ::testing::StrEq;
using ::testing::UnorderedElementsAre;

// TODO(yzhao): We'd better rewrite the test to use BCCWrapper directly, instead of
// SocketTraceConnector, to avoid triggering the userland parsing code, so these tests do not need
// change if we alter output format.

class SocketTraceBPFTest : public ::testing::Test {
 protected:
  void SetUp() override {
    FLAGS_stirling_disable_self_tracing = false;

    source_ = SocketTraceConnector::Create("socket_trace_connector");
    ASSERT_OK(source_->Init());

    // Create a context to pass into each TransferData() in the test, using a dummy ASID.
    static constexpr uint32_t kASID = 1;
    auto agent_metadata_state = std::make_shared<md::AgentMetadataState>(kASID);
    ctx_ = std::make_unique<ConnectorContext>(std::move(agent_metadata_state));

    TestOnlySetTargetPID(getpid());
  }

  void TearDown() override { ASSERT_OK(source_->Stop()); }

  std::string stmt_prepare_req;
  std::vector<std::string> stmt_prepare_resp;
  std::string stmt_execute_req;
  std::vector<std::string> stmt_execute_resp;
  std::string stmt_close_req;
  std::string query_req;
  std::vector<std::string> query_resp;

  testing::SendRecvScript GetPrepareExecuteScript() {
    testing::SendRecvScript prepare_execute_script;

    // Stmt Prepare
    stmt_prepare_req = mysql::testutils::GenRawPacket(mysql::testutils::GenStringRequest(
        mysql::testdata::kStmtPrepareRequest, mysql::MySQLEventType::kStmtPrepare));
    prepare_execute_script.push_back({stmt_prepare_req});
    prepare_execute_script.push_back({});
    std::deque<mysql::Packet> prepare_packets =
        mysql::testutils::GenStmtPrepareOKResponse(mysql::testdata::kStmtPrepareResponse);
    for (const auto& prepare_packet : prepare_packets) {
      stmt_prepare_resp.push_back(mysql::testutils::GenRawPacket(prepare_packet));
    }
    for (size_t i = 0; i < stmt_prepare_resp.size(); ++i) {
      prepare_execute_script[1].push_back(stmt_prepare_resp[i]);
    }

    // Stmt Execute
    stmt_execute_req = mysql::testutils::GenRawPacket(
        mysql::testutils::GenStmtExecuteRequest(mysql::testdata::kStmtExecuteRequest));
    prepare_execute_script.push_back({stmt_execute_req});
    prepare_execute_script.push_back({});
    std::deque<mysql::Packet> execute_packets =
        mysql::testutils::GenResultset(mysql::testdata::kStmtExecuteResultset);
    for (const auto& execute_packet : execute_packets) {
      stmt_execute_resp.push_back(mysql::testutils::GenRawPacket(execute_packet));
    }
    for (size_t i = 0; i < stmt_execute_resp.size(); ++i) {
      prepare_execute_script[3].push_back(stmt_execute_resp[i]);
    }

    // Stmt Close
    stmt_close_req = mysql::testutils::GenRawPacket(
        mysql::testutils::GenStmtCloseRequest(mysql::testdata::kStmtCloseRequest));
    prepare_execute_script.push_back({stmt_close_req});
    prepare_execute_script.push_back({});

    return prepare_execute_script;
  }

  testing::SendRecvScript GetQueryScript() {
    testing::SendRecvScript query_script;

    query_req = mysql::testutils::GenRawPacket(mysql::testutils::GenStringRequest(
        mysql::testdata::kQueryRequest, mysql::MySQLEventType::kQuery));
    query_script.push_back({query_req});
    query_script.push_back({});
    std::deque<mysql::Packet> query_packets =
        mysql::testutils::GenResultset(mysql::testdata::kQueryResultset);
    for (const auto& query_packet : query_packets) {
      query_resp.push_back(mysql::testutils::GenRawPacket(query_packet));
    }
    for (size_t i = 0; i < query_resp.size(); ++i) {
      query_script[1].push_back(query_resp[i]);
    }
    return query_script;
  }

  void ConfigureCapture(TrafficProtocol protocol, uint64_t mask) {
    auto* socket_trace_connector = dynamic_cast<SocketTraceConnector*>(source_.get());
    ASSERT_OK(socket_trace_connector->Configure(protocol, mask));
  }

  void TestOnlySetTargetPID(int64_t pid) {
    auto* socket_trace_connector = dynamic_cast<SocketTraceConnector*>(source_.get());
    ASSERT_OK(socket_trace_connector->TestOnlySetTargetPID(pid));
  }

  static constexpr std::string_view kHTTPReqMsg1 = R"(GET /endpoint1 HTTP/1.1
User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:67.0) Gecko/20100101 Firefox/67.0

)";

  static constexpr std::string_view kHTTPReqMsg2 = R"(GET /endpoint2 HTTP/1.1
User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:67.0) Gecko/20100101 Firefox/67.0

)";

  static constexpr std::string_view kHTTPRespMsg1 = R"(HTTP/1.1 200 OK
Content-Type: application/json; msg1
Content-Length: 0

)";

  static constexpr std::string_view kHTTPRespMsg2 = R"(HTTP/1.1 200 OK
Content-Type: application/json; msg2
Content-Length: 0

)";

  static constexpr std::string_view kNoProtocolMsg = R"(This is not an HTTP message)";

  static constexpr int kHTTPTableNum = SocketTraceConnector::kHTTPTableNum;

  static constexpr int kMySQLTableNum = SocketTraceConnector::kMySQLTableNum;
  static constexpr uint32_t kMySQLReqBodyIdx = kMySQLTable.ColIndex("req_body");

  std::unique_ptr<SourceConnector> source_;
  std::unique_ptr<ConnectorContext> ctx_;
};

TEST_F(SocketTraceBPFTest, Framework) {
  ConfigureCapture(kProtocolHTTP, kRoleRequestor | kRoleResponder);

  testing::SendRecvScript script({
      {kHTTPReqMsg1},
      {kHTTPRespMsg1},
      {kHTTPReqMsg2},
      {kHTTPRespMsg2},
  });
  testing::ClientServerSystem system;
  system.RunClientServer<&TCPSocket::Read, &TCPSocket::Write>(script);

  DataTable data_table(kHTTPTable);
  source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
  types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

  for (const std::shared_ptr<ColumnWrapper>& col : record_batch) {
    ASSERT_EQ(4, col->Size());
  }
}

// TODO(chengruizhe): Add test targeted at checking IPs.

TEST_F(SocketTraceBPFTest, WriteRespCapture) {
  ConfigureCapture(kProtocolHTTP, kRoleResponder);

  testing::ClientServerSystem system;
  system.RunClientServer<&TCPSocket::Read, &TCPSocket::Write>({{kHTTPRespMsg1, kHTTPRespMsg2}});

  {
    DataTable data_table(kHTTPTable);
    source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
    types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

    for (const std::shared_ptr<ColumnWrapper>& col : record_batch) {
      ASSERT_EQ(2, col->Size());
    }

    // These getpid() EXPECTs require docker container with --pid=host so that the container's PID
    // and the host machine are identical. See
    // https://stackoverflow.com/questions/33328841/pid-mapping-between-docker-and-host

    EXPECT_EQ(getpid(),
              md::UPID(record_batch[kHTTPUPIDIdx]->Get<types::UInt128Value>(0).val).pid());
    EXPECT_THAT(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(0), HasSubstr("msg1"));

    EXPECT_EQ(getpid(),
              md::UPID(record_batch[kHTTPUPIDIdx]->Get<types::UInt128Value>(1).val).pid());
    EXPECT_THAT(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(1), HasSubstr("msg2"));

    // Additional verifications. These are common to all HTTP1.x tracing, so we decide to not
    // duplicate them on all relevant tests.
    EXPECT_EQ(1, record_batch[kHTTPMajorVersionIdx]->Get<types::Int64Value>(0).val);
    EXPECT_EQ(static_cast<uint64_t>(HTTPContentType::kJSON),
              record_batch[kHTTPContentTypeIdx]->Get<types::Int64Value>(0).val);
    EXPECT_EQ(1, record_batch[kHTTPMajorVersionIdx]->Get<types::Int64Value>(1).val);
    EXPECT_EQ(static_cast<uint64_t>(HTTPContentType::kJSON),
              record_batch[kHTTPContentTypeIdx]->Get<types::Int64Value>(1).val);
  }

  // Check that MySQL table did not capture any data.
  {
    DataTable data_table(kMySQLTable);
    source_->TransferData(ctx_.get(), kMySQLTableNum, &data_table);
    types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

    for (const std::shared_ptr<ColumnWrapper>& col : record_batch) {
      ASSERT_EQ(0, col->Size());
    }
  }
}

TEST_F(SocketTraceBPFTest, SendRespCapture) {
  ConfigureCapture(TrafficProtocol::kProtocolHTTP, kRoleResponder);

  testing::ClientServerSystem system;
  system.RunClientServer<&TCPSocket::Recv, &TCPSocket::Send>({{kHTTPRespMsg1, kHTTPRespMsg2}});

  {
    DataTable data_table(kHTTPTable);
    source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
    types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

    for (const std::shared_ptr<ColumnWrapper>& col : record_batch) {
      ASSERT_EQ(2, col->Size());
    }

    // These 2 EXPECTs require docker container with --pid=host so that the container's PID and the
    // host machine are identical.
    // See https://stackoverflow.com/questions/33328841/pid-mapping-between-docker-and-host

    EXPECT_EQ(getpid(),
              md::UPID(record_batch[kHTTPUPIDIdx]->Get<types::UInt128Value>(0).val).pid());
    EXPECT_THAT(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(0), HasSubstr("msg1"));

    EXPECT_EQ(getpid(),
              md::UPID(record_batch[kHTTPUPIDIdx]->Get<types::UInt128Value>(1).val).pid());
    EXPECT_THAT(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(1), HasSubstr("msg2"));
  }

  // Check that MySQL table did not capture any data.
  {
    DataTable data_table(kHTTPTable);
    source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
    types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

    for (const std::shared_ptr<ColumnWrapper>& col : record_batch) {
      ASSERT_EQ(0, col->Size());
    }
  }
}

TEST_F(SocketTraceBPFTest, ReadRespCapture) {
  ConfigureCapture(TrafficProtocol::kProtocolHTTP, kRoleRequestor);

  testing::ClientServerSystem system;
  system.RunClientServer<&TCPSocket::Read, &TCPSocket::Write>({{kHTTPRespMsg1, kHTTPRespMsg2}});

  {
    DataTable data_table(kHTTPTable);
    source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
    types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

    for (const std::shared_ptr<ColumnWrapper>& col : record_batch) {
      ASSERT_EQ(2, col->Size());
    }

    // These 2 EXPECTs require docker container with --pid=host so that the container's PID and the
    // host machine are identical.
    // See https://stackoverflow.com/questions/33328841/pid-mapping-between-docker-and-host

    EXPECT_EQ(getpid(),
              md::UPID(record_batch[kHTTPUPIDIdx]->Get<types::UInt128Value>(0).val).pid());
    EXPECT_THAT(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(0), HasSubstr("msg1"));

    EXPECT_EQ(getpid(),
              md::UPID(record_batch[kHTTPUPIDIdx]->Get<types::UInt128Value>(1).val).pid());
    EXPECT_THAT(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(1), HasSubstr("msg2"));
  }

  // Check that MySQL table did not capture any data.
  {
    DataTable data_table(kMySQLTable);
    source_->TransferData(ctx_.get(), kMySQLTableNum, &data_table);
    types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

    for (const std::shared_ptr<ColumnWrapper>& col : record_batch) {
      ASSERT_EQ(0, col->Size());
    }
  }
}

TEST_F(SocketTraceBPFTest, RecvRespCapture) {
  ConfigureCapture(TrafficProtocol::kProtocolHTTP, kRoleRequestor);

  testing::ClientServerSystem system;
  system.RunClientServer<&TCPSocket::Recv, &TCPSocket::Send>({{kHTTPRespMsg1, kHTTPRespMsg2}});

  {
    DataTable data_table(kHTTPTable);
    source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
    types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

    for (const std::shared_ptr<ColumnWrapper>& col : record_batch) {
      ASSERT_EQ(2, col->Size());
    }

    // These 2 EXPECTs require docker container with --pid=host so that the container's PID and the
    // host machine are identical.
    // See https://stackoverflow.com/questions/33328841/pid-mapping-between-docker-and-host

    EXPECT_EQ(getpid(),
              md::UPID(record_batch[kHTTPUPIDIdx]->Get<types::UInt128Value>(0).val).pid());
    EXPECT_THAT(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(0), HasSubstr("msg1"));

    EXPECT_EQ(getpid(),
              md::UPID(record_batch[kHTTPUPIDIdx]->Get<types::UInt128Value>(1).val).pid());
    EXPECT_THAT(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(1), HasSubstr("msg2"));
  }

  // Check that MySQL table did not capture any data.
  {
    DataTable data_table(kMySQLTable);
    source_->TransferData(ctx_.get(), kMySQLTableNum, &data_table);
    types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

    for (const std::shared_ptr<ColumnWrapper>& col : record_batch) {
      ASSERT_EQ(0, col->Size());
    }
  }
}

TEST_F(SocketTraceBPFTest, MySQLStmtPrepareExecuteClose) {
  FLAGS_stirling_enable_mysql_tracing = true;

  ConfigureCapture(TrafficProtocol::kProtocolMySQL, kRoleRequestor);
  testing::ClientServerSystem system;
  system.RunClientServer<&TCPSocket::Read, &TCPSocket::Write>(GetPrepareExecuteScript());

  // Check that HTTP table did not capture any data.
  {
    DataTable data_table(kHTTPTable);
    source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
    types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

    for (const std::shared_ptr<ColumnWrapper>& col : record_batch) {
      ASSERT_EQ(0, col->Size());
    }
  }

  // Check that MySQL table did capture the appropriate data.
  {
    DataTable data_table(kMySQLTable);
    source_->TransferData(ctx_.get(), kMySQLTableNum, &data_table);
    types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

    for (const std::shared_ptr<ColumnWrapper>& col : record_batch) {
      ASSERT_EQ(3, col->Size());
    }

    EXPECT_EQ(
        "SELECT sock.sock_id AS id, GROUP_CONCAT(tag.name) AS tag_name FROM "
        "sock "
        "JOIN sock_tag ON "
        "sock.sock_id=sock_tag.sock_id JOIN tag ON sock_tag.tag_id=tag.tag_id WHERE tag.name=? "
        "GROUP "
        "BY id ORDER BY ?",
        record_batch[kMySQLReqBodyIdx]->Get<types::StringValue>(0));

    EXPECT_EQ(
        "SELECT sock.sock_id AS id, GROUP_CONCAT(tag.name) AS tag_name FROM "
        "sock "
        "JOIN sock_tag ON "
        "sock.sock_id=sock_tag.sock_id JOIN tag ON sock_tag.tag_id=tag.tag_id WHERE tag.name=brown "
        "GROUP "
        "BY id ORDER BY id",
        record_batch[kMySQLReqBodyIdx]->Get<types::StringValue>(1));

    EXPECT_EQ("", record_batch[kMySQLReqBodyIdx]->Get<types::StringValue>(2));
  }
}

TEST_F(SocketTraceBPFTest, MySQLQuery) {
  FLAGS_stirling_enable_mysql_tracing = true;

  ConfigureCapture(TrafficProtocol::kProtocolMySQL, kRoleRequestor);
  testing::ClientServerSystem system;
  system.RunClientServer<&TCPSocket::Read, &TCPSocket::Write>(GetQueryScript());

  // Check that HTTP table did not capture any data.
  {
    DataTable data_table(kHTTPTable);
    source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
    types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

    for (const std::shared_ptr<ColumnWrapper>& col : record_batch) {
      ASSERT_EQ(0, col->Size());
    }
  }

  // Check that MySQL table did capture the appropriate data.
  {
    DataTable data_table(kMySQLTable);
    source_->TransferData(ctx_.get(), kMySQLTableNum, &data_table);
    types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

    for (const std::shared_ptr<ColumnWrapper>& col : record_batch) {
      ASSERT_EQ(1, col->Size());
    }

    EXPECT_EQ("SELECT name FROM tag;", record_batch[kMySQLReqBodyIdx]->Get<types::StringValue>(0));
  }
}

TEST_F(SocketTraceBPFTest, NoProtocolWritesNotCaptured) {
  ConfigureCapture(TrafficProtocol::kProtocolHTTP, kRoleRequestor | kRoleResponder);
  ConfigureCapture(TrafficProtocol::kProtocolMySQL, kRoleRequestor);

  testing::ClientServerSystem system;
  system.RunClientServer<&TCPSocket::Read, &TCPSocket::Write>(
      {{kNoProtocolMsg, "", kNoProtocolMsg, ""}});

  // Check that HTTP table did not capture any data.
  {
    DataTable data_table(kHTTPTable);
    source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
    types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

    // Should not have captured anything.
    for (const std::shared_ptr<ColumnWrapper>& col : record_batch) {
      ASSERT_EQ(0, col->Size());
    }
  }

  // Check that MySQL table did not capture any data.
  {
    DataTable data_table(kMySQLTable);
    source_->TransferData(ctx_.get(), kMySQLTableNum, &data_table);
    types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

    // Should not have captured anything.
    for (const std::shared_ptr<ColumnWrapper>& col : record_batch) {
      ASSERT_EQ(0, col->Size());
    }
  }
}

TEST_F(SocketTraceBPFTest, MultipleConnections) {
  ConfigureCapture(TrafficProtocol::kProtocolHTTP, kRoleRequestor);

  // Two separate connections.
  testing::ClientServerSystem system1;
  system1.RunClientServer<&TCPSocket::Read, &TCPSocket::Write>({{kHTTPRespMsg1}});

  testing::ClientServerSystem system2;
  system2.RunClientServer<&TCPSocket::Read, &TCPSocket::Write>({{kHTTPRespMsg2}});

  {
    DataTable data_table(kHTTPTable);
    source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
    types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

    for (const std::shared_ptr<ColumnWrapper>& col : record_batch) {
      ASSERT_EQ(2, col->Size());
    }

    std::vector<std::pair<int64_t, std::string>> results;
    for (int i = 0; i < 2; ++i) {
      results.emplace_back(std::make_pair(
          md::UPID(record_batch[kHTTPUPIDIdx]->Get<types::UInt128Value>(i).val).pid(),
          record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(i)));
    }

    EXPECT_THAT(results, UnorderedElementsAre(Pair(getpid(), HasSubstr("msg1")),
                                              Pair(getpid(), HasSubstr("msg2"))));
  }
}

TEST_F(SocketTraceBPFTest, StartTime) {
  ConfigureCapture(TrafficProtocol::kProtocolHTTP, kRoleRequestor);

  testing::ClientServerSystem system;
  system.RunClientServer<&TCPSocket::Recv, &TCPSocket::Send>({{kHTTPRespMsg1, kHTTPRespMsg2}});

  // Kernel uses monotonic clock as start_time, so we must do the same.
  auto now = std::chrono::steady_clock::now();

  // Use a time window to make sure the recorded PID start_time is right.
  // Being super generous with the window, just in case test runs slow.
  auto time_window_start_tp = now - std::chrono::minutes(30);
  auto time_window_end_tp = now + std::chrono::minutes(5);

  // Start times are reported by Linux in what is essentially 10 ms units.
  constexpr int64_t kNsecondsPerSecond = 1000 * 1000 * 1000;
  constexpr int64_t kClockTicks = 100;
  constexpr int64_t kDivFactor = kNsecondsPerSecond / kClockTicks;

  auto time_window_start = time_window_start_tp.time_since_epoch().count() / kDivFactor;
  auto time_window_end = time_window_end_tp.time_since_epoch().count() / kDivFactor;

  DataTable data_table(kHTTPTable);
  source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
  types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

  ASSERT_EQ(2, record_batch[0]->Size());

  md::UPID upid0(record_batch[kHTTPUPIDIdx]->Get<types::UInt128Value>(0).val);
  EXPECT_EQ(getpid(), upid0.pid());
  EXPECT_LT(time_window_start, upid0.start_ts());
  EXPECT_GT(time_window_end, upid0.start_ts());

  md::UPID upid1(record_batch[kHTTPUPIDIdx]->Get<types::UInt128Value>(1).val);
  EXPECT_EQ(getpid(), upid1.pid());
  EXPECT_LT(time_window_start, upid1.start_ts());
  EXPECT_GT(time_window_end, upid1.start_ts());
}

// TODO(yzhao): Apply this pattern to other syscall pairs. An issue is that other syscalls do not
// use scatter buffer. One approach would be to concatenate inner vector to a single string, and
// then feed to the syscall. Another caution is that value-parameterized tests actually discourage
// changing functions being tested according to test parameters. The canonical pattern is using test
// parameters as inputs, but keep the function being tested fixed.
enum class SyscallPair {
  kSendRecvMsg,
  kWriteReadv,
};

class SyscallPairBPFTest : public SocketTraceBPFTest,
                           public ::testing::WithParamInterface<SyscallPair> {};

TEST_P(SyscallPairBPFTest, EventsAreCaptured) {
  ConfigureCapture(kProtocolHTTP, kRoleRequestor | kRoleResponder);
  testing::ClientServerSystem system({});
  const std::vector<std::vector<std::string_view>> data = {
      {"HTTP/1.1 200 OK\r\n", "Content-Type: json\r\n", "Content-Length: 1\r\n\r\na"},
      {"HTTP/1.1 404 Not Found\r\n", "Content-Type: json\r\n", "Content-Length: 2\r\n\r\nbc"}};
  switch (GetParam()) {
    case SyscallPair::kSendRecvMsg:
      system.RunClientServer<&TCPSocket::RecvMsg, &TCPSocket::SendMsg>(data);
      break;
    case SyscallPair::kWriteReadv:
      system.RunClientServer<&TCPSocket::ReadV, &TCPSocket::WriteV>(data);
      break;
  }

  DataTable data_table(kHTTPTable);
  source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
  types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

  for (const std::shared_ptr<ColumnWrapper>& col : record_batch) {
    // 2 for sendmsg() and 2 for recvmsg().
    ASSERT_EQ(4, col->Size());
  }

  for (int i : {0, 2}) {
    EXPECT_EQ(200, record_batch[kHTTPRespStatusIdx]->Get<types::Int64Value>(i).val);
    EXPECT_THAT(std::string(record_batch[kHTTPRespBodyIdx]->Get<types::StringValue>(i)),
                StrEq("a"));
    EXPECT_THAT(std::string(record_batch[kHTTPRespMessageIdx]->Get<types::StringValue>(i)),
                StrEq("OK"));
  }
  for (int i : {1, 3}) {
    EXPECT_EQ(404, record_batch[kHTTPRespStatusIdx]->Get<types::Int64Value>(i).val);
    EXPECT_THAT(std::string(record_batch[kHTTPRespBodyIdx]->Get<types::StringValue>(i)),
                StrEq("bc"));
    EXPECT_THAT(std::string(record_batch[kHTTPRespMessageIdx]->Get<types::StringValue>(i)),
                StrEq("Not Found"));
  }
}

INSTANTIATE_TEST_CASE_P(IOVecSyscalls, SyscallPairBPFTest,
                        ::testing::Values(SyscallPair::kSendRecvMsg, SyscallPair::kWriteReadv));

}  // namespace stirling
}  // namespace pl
