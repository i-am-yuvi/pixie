#include <gmock/gmock.h>
#include <google/protobuf/util/message_differencer.h>
#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>
#include <sys/types.h>
#include <unistd.h>

extern "C" {
#include <nghttp2/nghttp2_frame.h>
}

#include <thread>

#include "src/common/subprocess/subprocess.h"
#include "src/common/testing/testing.h"
#include "src/stirling/data_table.h"
#include "src/stirling/grpc.h"
#include "src/stirling/socket_trace_connector.h"
#include "src/stirling/testing/greeter_server.h"
#include "src/stirling/testing/grpc_stub.h"
#include "src/stirling/testing/proto/greet.grpc.pb.h"

namespace pl {
namespace stirling {
namespace grpc {

using ::grpc::Channel;
using ::pl::stirling::testing::CreateInsecureGRPCChannel;
using ::pl::stirling::testing::Greeter;
using ::pl::stirling::testing::Greeter2;
using ::pl::stirling::testing::Greeter2Service;
using ::pl::stirling::testing::GreeterService;
using ::pl::stirling::testing::GRPCStub;
using ::pl::stirling::testing::HelloReply;
using ::pl::stirling::testing::HelloRequest;
using ::pl::stirling::testing::ServiceRunner;
using ::pl::stirling::testing::StreamingGreeter;
using ::pl::stirling::testing::StreamingGreeterService;
using ::pl::testing::proto::EqualsProto;
using ::pl::types::ColumnWrapperRecordBatch;
using ::testing::AnyOf;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::MatchesRegex;
using ::testing::SizeIs;
using ::testing::StrEq;

constexpr int kHTTPTableNum = SocketTraceConnector::kHTTPTableNum;
constexpr DataTableSchema kHTTPTable = SocketTraceConnector::kHTTPTable;
constexpr uint32_t kHTTPMajorVersionIdx = kHTTPTable.ColIndex("http_major_version");
constexpr uint32_t kHTTPContentTypeIdx = kHTTPTable.ColIndex("http_content_type");
constexpr uint32_t kHTTPReqHeadersIdx = kHTTPTable.ColIndex("http_req_headers");
constexpr uint32_t kHTTPRespHeadersIdx = kHTTPTable.ColIndex("http_resp_headers");
constexpr uint32_t kHTTPPIDIdx = kHTTPTable.ColIndex("pid");
constexpr uint32_t kHTTPRemoteAddrIdx = kHTTPTable.ColIndex("remote_addr");
constexpr uint32_t kHTTPRemotePortIdx = kHTTPTable.ColIndex("remote_port");
constexpr uint32_t kHTTPReqBodyIdx = kHTTPTable.ColIndex("http_req_body");
constexpr uint32_t kHTTPRespBodyIdx = kHTTPTable.ColIndex("http_resp_body");

std::vector<size_t> FindRecordIdxMatchesPid(const ColumnWrapperRecordBatch& http_record, int pid) {
  std::vector<size_t> res;
  for (size_t i = 0; i < http_record[kHTTPPIDIdx]->Size(); ++i) {
    if (http_record[kHTTPPIDIdx]->Get<types::Int64Value>(i).val == pid) {
      res.push_back(i);
    }
  }
  return res;
}

HelloReply GetHelloReply(const ColumnWrapperRecordBatch& record_batch, const size_t idx) {
  HelloReply received_reply;
  std::string msg = record_batch[kHTTPRespBodyIdx]->Get<types::StringValue>(idx);
  if (!msg.empty()) {
    received_reply.ParseFromString(msg.substr(kGRPCMessageHeaderSizeInBytes));
  }
  return received_reply;
}

HelloRequest GetHelloRequest(const ColumnWrapperRecordBatch& record_batch, const size_t idx) {
  HelloRequest received_reply;
  std::string msg = record_batch[kHTTPReqBodyIdx]->Get<types::StringValue>(idx);
  if (!msg.empty()) {
    received_reply.ParseFromString(msg.substr(kGRPCMessageHeaderSizeInBytes));
  }
  return received_reply;
}

TEST(GRPCTraceBPFTest, TestGolangGrpcService) {
  // Force disable protobuf parsing to output the binary protobuf in record batch.
  // Also ensure test remain passing when the default changes.
  FLAGS_enable_parsing_protobufs = false;

  // Bump perf buffer size to 1MiB to avoid perf buffer overflow.
  FLAGS_stirling_bpf_perf_buffer_page_count = 256;

  constexpr char kBaseDir[] = "src/stirling/testing";
  std::string s_path =
      TestEnvironment::PathToTestDataFile(absl::StrCat(kBaseDir, "/go_greeter_server"));
  std::string c_path =
      TestEnvironment::PathToTestDataFile(absl::StrCat(kBaseDir, "/go_greeter_client"));
  SubProcess s({s_path});
  EXPECT_OK(s.Start());

  // TODO(yzhao): We have to install probes after starting server. Otherwise we will run into
  // failures when detaching them. This might be relevant to probes are inherited by child process
  // when fork() and execvp().
  std::unique_ptr<SourceConnector> connector =
      SocketTraceConnector::Create("socket_trace_connector");
  auto* socket_trace_connector = static_cast<SocketTraceConnector*>(connector.get());
  ASSERT_NE(nullptr, socket_trace_connector);
  ASSERT_OK(connector->Init());

  // TODO(yzhao): Add a --count flag to greeter client so we can test the case of multiple RPC calls
  // (multiple HTTP2 streams).
  SubProcess c({c_path, "-name=PixieLabs", "-once"});
  EXPECT_OK(c.Start());

  EXPECT_OK(socket_trace_connector->TestOnlySetTargetPID(c.child_pid()));

  EXPECT_EQ(0, c.Wait()) << "Client should exit normally.";
  s.Kill();
  EXPECT_EQ(9, s.Wait()) << "Server should have been killed.";

  DataTable data_table(SocketTraceConnector::kHTTPTable);
  types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

  connector->TransferData(/* ctx */ nullptr, kHTTPTableNum, &data_table);
  for (const auto& col : record_batch) {
    // Sometimes connect() returns 0, so we might have data from requester and responder.
    ASSERT_GE(col->Size(), 1);
  }
  const std::vector<size_t> target_record_indices =
      FindRecordIdxMatchesPid(record_batch, c.child_pid());
  // We should get exactly one record.
  ASSERT_THAT(target_record_indices, SizeIs(1));
  const size_t target_record_idx = target_record_indices.front();

  EXPECT_THAT(
      std::string(record_batch[kHTTPReqHeadersIdx]->Get<types::StringValue>(target_record_idx)),
      MatchesRegex(":authority: localhost:50051\n"
                   ":method: POST\n"
                   ":path: /pl.stirling.testing.Greeter/SayHello\n"
                   ":scheme: http\n"
                   "content-type: application/grpc\n"
                   "grpc-timeout: [0-9a-zA-Z]+u\n"
                   "te: trailers\n"
                   "user-agent: grpc-go/.+"));
  EXPECT_THAT(
      std::string(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(target_record_idx)),
      MatchesRegex(":status: 200\n"
                   "content-type: application/grpc\n"
                   "grpc-message: \n"
                   "grpc-status: 0"));
  EXPECT_THAT(
      std::string(record_batch[kHTTPRemoteAddrIdx]->Get<types::StringValue>(target_record_idx)),
      HasSubstr("127.0.0.1"));
  EXPECT_EQ(50051, record_batch[kHTTPRemotePortIdx]->Get<types::Int64Value>(target_record_idx).val);
  EXPECT_EQ(2, record_batch[kHTTPMajorVersionIdx]->Get<types::Int64Value>(target_record_idx).val);
  EXPECT_EQ(static_cast<uint64_t>(HTTPContentType::kGRPC),
            record_batch[kHTTPContentTypeIdx]->Get<types::Int64Value>(target_record_idx).val);

  EXPECT_THAT(GetHelloReply(record_batch, target_record_idx),
              EqualsProto(R"proto(message: "Hello PixieLabs")proto"));
}

class GRPCCppTest : public ::testing::Test {
 protected:
  void SetUp() override {
    SetUpSocketTraceConnector();
    SetUpGRPCServices();
  }

  void SetUpSocketTraceConnector() {
    // Force disable protobuf parsing to output the binary protobuf in record batch.
    // Also ensure test remain passing when the default changes.
    FLAGS_enable_parsing_protobufs = false;

    source_ = SocketTraceConnector::Create("bcc_grpc_trace");
    ASSERT_OK(source_->Init());

    auto* socket_trace_connector = static_cast<SocketTraceConnector*>(source_.get());
    ASSERT_NE(nullptr, socket_trace_connector);

    data_table_ = std::make_unique<DataTable>(SocketTraceConnector::kHTTPTable);
  }

  void SetUpGRPCServices() {
    runner_.RegisterService(&greeter_service_);
    runner_.RegisterService(&greeter2_service_);
    runner_.RegisterService(&streaming_greeter_service_);

    server_ = runner_.Run();

    auto* server_ptr = server_.get();
    server_thread_ = std::thread([server_ptr]() { server_ptr->Wait(); });

    client_channel_ = CreateInsecureGRPCChannel(absl::StrCat("127.0.0.1:", runner_.port()));
    greeter_stub_ = std::make_unique<GRPCStub<Greeter>>(client_channel_);
    greeter2_stub_ = std::make_unique<GRPCStub<Greeter2>>(client_channel_);
    streaming_greeter_stub_ = std::make_unique<GRPCStub<StreamingGreeter>>(client_channel_);
  }

  void TearDown() override {
    ASSERT_OK(source_->Stop());
    server_->Shutdown();
    if (server_thread_.joinable()) {
      server_thread_.join();
    }
  }

  template <typename StubType, typename RPCMethodType>
  std::vector<::grpc::Status> CallRPC(StubType* stub, RPCMethodType method,
                                      const std::vector<std::string>& names) {
    std::vector<::grpc::Status> res;
    HelloRequest req;
    HelloReply resp;
    for (const auto& n : names) {
      req.set_name(n);
      res.push_back(stub->CallRPC(method, req, &resp));
    }
    return res;
  }

  std::unique_ptr<SourceConnector> source_;
  std::unique_ptr<DataTable> data_table_;

  GreeterService greeter_service_;
  Greeter2Service greeter2_service_;
  StreamingGreeterService streaming_greeter_service_;

  ServiceRunner runner_;
  std::unique_ptr<::grpc::Server> server_;
  std::thread server_thread_;

  std::shared_ptr<Channel> client_channel_;

  std::unique_ptr<GRPCStub<Greeter>> greeter_stub_;
  std::unique_ptr<GRPCStub<Greeter2>> greeter2_stub_;
  std::unique_ptr<GRPCStub<StreamingGreeter>> streaming_greeter_stub_;
};

TEST_F(GRPCCppTest, MixedGRPCServicesOnSameGRPCChannel) {
  // TODO(yzhao): Put CallRPC() calls inside multiple threads. That would cause header parsing
  // failures, debug and fix the root cause.
  CallRPC(greeter_stub_.get(), &Greeter::Stub::SayHello, {"pixielabs", "pixielabs", "pixielabs"});
  CallRPC(greeter_stub_.get(), &Greeter::Stub::SayHelloAgain,
          {"pixielabs", "pixielabs", "pixielabs"});
  CallRPC(greeter2_stub_.get(), &Greeter2::Stub::SayHi, {"pixielabs", "pixielabs", "pixielabs"});
  CallRPC(greeter2_stub_.get(), &Greeter2::Stub::SayHiAgain,
          {"pixielabs", "pixielabs", "pixielabs"});
  source_->TransferData(/* ctx */ nullptr, kHTTPTableNum, data_table_.get());

  types::ColumnWrapperRecordBatch& record_batch = *data_table_->ActiveRecordBatch();
  std::vector<size_t> indices = FindRecordIdxMatchesPid(record_batch, getpid());
  EXPECT_THAT(indices, SizeIs(12));

  for (size_t idx : indices) {
    EXPECT_THAT(std::string(record_batch[kHTTPReqHeadersIdx]->Get<types::StringValue>(idx)),
                MatchesRegex(":authority: 127.0.0.1:[0-9]+\n"
                             ":method: POST\n"
                             ":path: /pl.stirling.testing.Greeter(|2)/Say(Hi|Hello)(|Again)\n"
                             ":scheme: http\n"
                             "accept-encoding: identity,gzip\n"
                             "content-type: application/grpc\n"
                             "grpc-accept-encoding: identity,deflate,gzip\n"
                             "grpc-timeout: [0-9a-zA-Z]+\n"
                             "te: trailers\n"
                             "user-agent: .*"));
    EXPECT_THAT(std::string(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(idx)),
                MatchesRegex(":status: 200\n"
                             "accept-encoding: identity,gzip\n"
                             "content-type: application/grpc\n"
                             "grpc-accept-encoding: identity,deflate,gzip\n"
                             "grpc-status: 0"));
    EXPECT_THAT(std::string(record_batch[kHTTPRemoteAddrIdx]->Get<types::StringValue>(idx)),
                HasSubstr("127.0.0.1"));
    EXPECT_EQ(runner_.port(), record_batch[kHTTPRemotePortIdx]->Get<types::Int64Value>(idx).val);
    EXPECT_EQ(2, record_batch[kHTTPMajorVersionIdx]->Get<types::Int64Value>(idx).val);
    EXPECT_EQ(static_cast<uint64_t>(HTTPContentType::kGRPC),
              record_batch[kHTTPContentTypeIdx]->Get<types::Int64Value>(idx).val);
    EXPECT_THAT(GetHelloReply(record_batch, idx),
                AnyOf(EqualsProto(R"proto(message: "Hello pixielabs!")proto"),
                      EqualsProto(R"proto(message: "Hi pixielabs!")proto")));
  }
}

// Tests to show the captured results from a timed out RPC call.
TEST_F(GRPCCppTest, RPCTimesOut) {
  greeter_service_.set_enable_cond_wait(true);
  auto statuses = CallRPC(greeter_stub_.get(), &Greeter::Stub::SayHello, {"pixielabs"});
  ASSERT_THAT(statuses, SizeIs(1));
  EXPECT_EQ(::grpc::StatusCode::DEADLINE_EXCEEDED, statuses[0].error_code());

  source_->TransferData(/* ctx */ nullptr, kHTTPTableNum, data_table_.get());

  types::ColumnWrapperRecordBatch& record_batch = *data_table_->ActiveRecordBatch();
  std::vector<size_t> indices = FindRecordIdxMatchesPid(record_batch, getpid());
  // TODO(yzhao): ATM missing response, here because of response times out, renders requests being
  // held in buffer and not exported. Change to export requests after a certain timeout.
  EXPECT_THAT(indices, IsEmpty());

  // Wait for RPC call to timeout, and then unblock the server.
  greeter_service_.Notify();
}

std::vector<HelloReply> ParseProtobufRecords(absl::string_view buf) {
  std::vector<HelloReply> res;
  while (!buf.empty()) {
    const uint32_t len = nghttp2_get_uint32(reinterpret_cast<const uint8_t*>(buf.data()) + 1);
    HelloReply reply;
    reply.ParseFromArray(buf.data() + kGRPCMessageHeaderSizeInBytes, len);
    res.push_back(std::move(reply));
    buf.remove_prefix(kGRPCMessageHeaderSizeInBytes + len);
  }
  return res;
}

// Tests that a streaming RPC call will keep a HTTP2 stream open for the entirety of the RPC call.
// Therefore if the server takes a long time to return the results, the trace record would not
// be exported until then.
// TODO(yzhao): We need some way to export streaming RPC trace record gradually.
TEST_F(GRPCCppTest, ServerStreamingRPC) {
  HelloRequest req;
  req.set_name("pixielabs");
  req.set_count(3);

  std::vector<HelloReply> replies;

  ::grpc::Status st = streaming_greeter_stub_->CallServerStreamingRPC(
      &StreamingGreeter::Stub::SayHello, req, &replies);
  EXPECT_TRUE(st.ok());
  EXPECT_THAT(replies, SizeIs(3));

  source_->TransferData(/* ctx */ nullptr, kHTTPTableNum, data_table_.get());

  types::ColumnWrapperRecordBatch& record_batch = *data_table_->ActiveRecordBatch();
  std::vector<size_t> indices = FindRecordIdxMatchesPid(record_batch, getpid());
  EXPECT_THAT(indices, SizeIs(1));

  for (size_t idx : indices) {
    std::vector<std::string> header_fields =
        absl::StrSplit(record_batch[kHTTPReqHeadersIdx]->Get<types::StringValue>(idx), "\n");
    EXPECT_THAT(
        header_fields,
        ElementsAre(MatchesRegex(":authority: 127.0.0.1:[0-9]+"), ":method: POST",
                    ":path: /pl.stirling.testing.StreamingGreeter/SayHello", ":scheme: http",
                    "accept-encoding: identity,gzip", "content-type: application/grpc",
                    "grpc-accept-encoding: identity,deflate,gzip",
                    MatchesRegex("grpc-timeout: [0-9a-zA-Z]+"), "te: trailers",
                    MatchesRegex("user-agent: .*")));
    header_fields =
        absl::StrSplit(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(idx), "\n");
    EXPECT_THAT(header_fields,
                ElementsAre(":status: 200", "accept-encoding: identity,gzip",
                            "content-type: application/grpc",
                            "grpc-accept-encoding: identity,deflate,gzip", "grpc-status: 0"));
    EXPECT_THAT(GetHelloRequest(record_batch, idx),
                EqualsProto(R"proto(name: "pixielabs" count: 3)proto"));
    EXPECT_THAT(ParseProtobufRecords(record_batch[kHTTPRespBodyIdx]->Get<types::StringValue>(idx)),
                ElementsAre(EqualsProto("message: 'Hello pixielabs for no. 0!'"),
                            EqualsProto("message: 'Hello pixielabs for no. 1!'"),
                            EqualsProto("message: 'Hello pixielabs for no. 2!'")));
  }
}

// Do not initialize socket tracer to simulate the socket tracer missing head of start of the HTTP2
// connection.
class GRPCCppMiddleInterceptTest : public GRPCCppTest {
 protected:
  void SetUp() { SetUpGRPCServices(); }
};

TEST_F(GRPCCppMiddleInterceptTest, InterceptMiddleOfTheConnection) {
  CallRPC(greeter_stub_.get(), &Greeter::Stub::SayHello, {"pixielabs", "pixielabs", "pixielabs"});

  // Attach the probes after connection started.
  SetUpSocketTraceConnector();
  CallRPC(greeter_stub_.get(), &Greeter::Stub::SayHello, {"pixielabs", "pixielabs", "pixielabs"});
  source_->TransferData(/* ctx */ nullptr, kHTTPTableNum, data_table_.get());

  types::ColumnWrapperRecordBatch& record_batch = *data_table_->ActiveRecordBatch();
  std::vector<size_t> indices = FindRecordIdxMatchesPid(record_batch, getpid());
  EXPECT_THAT(indices, SizeIs(3));
  for (size_t idx : indices) {
    // Header parsing would fail, because missing the head of start.
    // TODO(yzhao): We should device some meaningful mechanism for capturing headers, if inflation
    // failed.
    EXPECT_THAT(GetHelloReply(record_batch, idx),
                AnyOf(EqualsProto(R"proto(message: "Hello pixielabs!")proto"),
                      EqualsProto(R"proto(message: "Hi pixielabs!")proto")));
  }
}

class GRPCCppCallingNonRegisteredServiceTest : public GRPCCppTest {
 protected:
  void SetUp() {
    SetUpSocketTraceConnector();

    runner_.RegisterService(&greeter2_service_);
    server_ = runner_.Run();

    auto* server_ptr = server_.get();
    server_thread_ = std::thread([server_ptr]() { server_ptr->Wait(); });

    client_channel_ = CreateInsecureGRPCChannel(absl::StrCat("127.0.0.1:", runner_.port()));
    greeter_stub_ = std::make_unique<GRPCStub<Greeter>>(client_channel_);
  }
};

// Tests to show what is captured when calling a remote endpoint that does not implement the
// requested method.
TEST_F(GRPCCppCallingNonRegisteredServiceTest, ResultsAreAsExpected) {
  CallRPC(greeter_stub_.get(), &Greeter::Stub::SayHello, {"pixielabs", "pixielabs", "pixielabs"});
  source_->TransferData(/* ctx */ nullptr, kHTTPTableNum, data_table_.get());
  types::ColumnWrapperRecordBatch& record_batch = *data_table_->ActiveRecordBatch();
  std::vector<size_t> indices = FindRecordIdxMatchesPid(record_batch, getpid());
  EXPECT_THAT(indices, SizeIs(3));
  for (size_t idx : indices) {
    EXPECT_THAT(std::string(record_batch[kHTTPReqHeadersIdx]->Get<types::StringValue>(idx)),
                MatchesRegex(":authority: 127.0.0.1:[0-9]+\n"
                             ":method: POST\n"
                             ":path: /pl.stirling.testing.Greeter(|2)/Say(Hi|Hello)(|Again)\n"
                             ":scheme: http\n"
                             "accept-encoding: identity,gzip\n"
                             "content-type: application/grpc\n"
                             "grpc-accept-encoding: identity,deflate,gzip\n"
                             "grpc-timeout: [0-9a-zA-Z]+\n"
                             "te: trailers\n"
                             "user-agent: .*"));
    EXPECT_THAT(std::string(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(idx)),
                MatchesRegex(":status: 200\n"
                             "content-type: application/grpc\n"
                             "grpc-status: 12"));
    EXPECT_THAT(GetHelloRequest(record_batch, idx), EqualsProto(R"proto(name: "pixielabs")proto"));
    EXPECT_THAT(record_batch[kHTTPRespBodyIdx]->Get<types::StringValue>(idx), IsEmpty());
  }
}

}  // namespace grpc
}  // namespace stirling
}  // namespace pl
