#include <math.h>
#include <deque>
#include <string>

#include "src/common/base/byte_utils.h"
#include "src/stirling/mysql/mysql.h"
#include "src/stirling/mysql/test_utils.h"

namespace pl {
namespace stirling {
namespace mysql {
namespace testutils {

/**
 * These Gen functions help generate raw string or Packet needed for testing the MySQL parser
 * or stitcher, respectively. The caller are expected to use structured events in test_data.h
 * and revert them back to strings or packets as test input.
 */

/**
 * Generates raw MySQL packets in the form of a string.
 */
std::string GenRawPacket(uint8_t packet_num, const std::string& msg) {
  char header[4];
  utils::IntToLEBytes(msg.size(), header);
  header[3] = packet_num;
  return absl::StrCat(std::string_view(header, 4), msg);
}

/**
 * Generates a raw packet with a string request.
 */
std::string GenRequest(char command, const std::string& msg) {
  return GenRawPacket(0, absl::StrCat(std::string(1, command), msg));
}

/**
 * Generates the bytes of a length-encoded integer.
 * https://dev.mysql.com/doc/internals/en/integer.html#length-encoded-integer
 */
std::string GenLengthEncodedInt(int num) {
  DCHECK(num < pow(2, 64));
  if (num < 251) {
    char count_bytes[1];
    utils::IntToLEBytes(num, count_bytes);
    return std::string(count_bytes, 1);
  } else if (num < pow(2, 16)) {
    char count_bytes[2];
    utils::IntToLEBytes(num, count_bytes);
    return absl::StrCat("fc", std::string(count_bytes, 2));
  } else if (num < pow(2, 24)) {
    char count_bytes[3];
    utils::IntToLEBytes(num, count_bytes);
    return absl::StrCat("fd", std::string(count_bytes, 3));
  } else {
    char count_bytes[8];
    utils::IntToLEBytes(num, count_bytes);
    return absl::StrCat("fe", std::string(count_bytes, 8));
  }
}

/**
 * Generates the header packet of Resultset response. It contains num of cols.
 */
Packet GenCountPacket(int num_col) {
  std::string msg = GenLengthEncodedInt(num_col);
  return Packet{0, std::chrono::steady_clock::now(), std::move(msg), MySQLEventType::kUnknown};
}

/**
 * Generates a Col Definition packet. Can be used in StmtPrepareResponse or Resultset.
 */
Packet GenColDefinition(const ColDefinition& col_def) {
  return Packet{0, std::chrono::steady_clock::now(), std::move(col_def.msg),
                MySQLEventType::kUnknown};
}

/**
 * Generates a resultset row.
 */
Packet GenResultsetRow(const ResultsetRow& row) {
  return Packet{0, std::chrono::steady_clock::now(), std::move(row.msg), MySQLEventType::kUnknown};
}

/**
 * Generates a header of StmtPrepare Response.
 */
Packet GenStmtPrepareRespHeader(const StmtPrepareRespHeader& header) {
  char statement_id[4];
  char num_columns[2];
  char num_params[2];
  char warning_count[2];
  utils::IntToLEBytes(header.stmt_id, statement_id);
  utils::IntToLEBytes(header.num_columns, num_columns);
  utils::IntToLEBytes(header.num_params, num_params);
  utils::IntToLEBytes(header.warning_count, warning_count);
  std::string msg = absl::StrCat(ConstStringView("\x00"), std::string(statement_id, 4),
                                 std::string(num_columns, 2), std::string(num_params, 2),
                                 ConstStringView("\x00"), std::string(warning_count, 2));

  return Packet{0, std::chrono::steady_clock::now(), std::move(msg), MySQLEventType::kUnknown};
}

/**
 * Generates a deque of packets. Contains a col counter packet and n resultset rows.
 */
std::deque<Packet> GenResultset(const Resultset& resultset, bool client_eof_deprecate) {
  std::deque<Packet> result;
  auto resp_header = GenCountPacket(resultset.num_col());
  result.emplace_back(std::move(resp_header));
  for (ColDefinition col_def : resultset.col_defs()) {
    result.emplace_back(GenColDefinition(col_def));
  }
  if (!client_eof_deprecate) {
    result.emplace_back(GenEOF());
  }
  for (ResultsetRow row : resultset.results()) {
    result.emplace_back(GenResultsetRow(row));
  }
  if (client_eof_deprecate) {
    result.emplace_back(GenOK());
  } else {
    result.emplace_back(GenEOF());
  }
  return result;
}

/**
 * Generates a StmtPrepareOkResponse.
 */
std::deque<Packet> GenStmtPrepareOKResponse(const StmtPrepareOKResponse& resp) {
  std::deque<Packet> result;
  auto resp_header = GenStmtPrepareRespHeader(resp.resp_header());
  result.push_back(resp_header);

  for (ColDefinition param_def : resp.param_defs()) {
    ColDefinition p{std::move(param_def.msg)};
    result.push_back(GenColDefinition(p));
  }
  result.push_back(GenEOF());

  for (ColDefinition col_def : resp.col_defs()) {
    ColDefinition c{std::move(col_def.msg)};
    result.push_back(GenColDefinition(c));
  }
  result.push_back(GenEOF());
  return result;
}

Packet GenStmtExecuteRequest(const StmtExecuteRequest& req) {
  char statement_id[4];
  utils::IntToLEBytes(req.stmt_id(), statement_id);
  std::string msg = absl::StrCat(std::string(1, kComStmtExecute), std::string(statement_id, 4),
                                 ConstStringView("\x00\x01\x00\x00\x00"));
  int num_params = req.params().size();
  if (num_params > 0) {
    for (int i = 0; i < (num_params + 7) / 8; i++) {
      msg += std::string("\x00", 1);
    }
    msg += "\x01";
  }
  for (ParamPacket param : req.params()) {
    switch (param.type) {
      // TODO(chengruizhe): Add more types.
      case StmtExecuteParamType::kString:
        msg += std::string("\xfe\x00", 2);
        break;
      default:
        msg += std::string("\xfe\x00", 2);
        break;
    }
  }
  for (ParamPacket param : req.params()) {
    msg += GenLengthEncodedInt(param.value.size());
    msg += param.value;
  }
  return Packet{0, std::chrono::steady_clock::now(), std::move(msg), MySQLEventType::kStmtExecute};
}

Packet GenStmtCloseRequest(const StmtCloseRequest& req) {
  char statement_id[4];
  utils::IntToLEBytes(req.stmt_id(), statement_id);
  std::string msg = absl::StrCat(std::string(1, kComStmtClose), std::string(statement_id, 4));
  return Packet{0, std::chrono::steady_clock::now(), std::move(msg), MySQLEventType::kStmtClose};
}

/**
 * Generates a String Request packet. Takes in an eventType for now.
 * TODO(chengruizhe): Remove MySQLEventType once type is inferred in stitcher.
 */
Packet GenStringRequest(const StringRequest& req, MySQLEventType type) {
  char command;
  switch (type) {
    case MySQLEventType::kStmtPrepare:
      command = kComStmtPrepare;
      break;
    case MySQLEventType::kStmtExecute:
      command = kComStmtExecute;
      break;
    case MySQLEventType::kQuery:
      command = kComQuery;
      break;
    default:
      LOG(FATAL) << "Unknown type for string request.";
      break;
  }
  return Packet{0, std::chrono::steady_clock::now(),
                absl::StrCat(std::string(1, command), req.msg()), type};
}

/**
 * Generates a Err packet.
 */
Packet GenErr(const ErrResponse& err) {
  char error_code[2];
  utils::IntToLEBytes(err.error_code(), error_code);
  std::string msg = absl::StrCat("\xff", std::string(error_code, 2), "\x23\x48\x59\x30\x30\x30",
                                 err.error_message());
  return Packet{0, std::chrono::steady_clock::now(), std::move(msg), MySQLEventType::kUnknown};
}

/**
 * Generates a OK packet. Content is fixed.
 */
Packet GenOK() {
  std::string msg = std::string(ConstStringView("\x00\x00\x00\x02\x00\x00\x00"));
  return Packet{0, std::chrono::steady_clock::now(), std::move(msg), MySQLEventType::kUnknown};
}

/**
 * Generates a EOF packet. Content is fixed.
 */
Packet GenEOF() {
  std::string msg = std::string(ConstStringView("\xfe\x00\x00\x22\x00"));
  return Packet{0, std::chrono::steady_clock::now(), std::move(msg), MySQLEventType::kUnknown};
}

}  // namespace testutils
}  // namespace mysql
}  // namespace stirling
}  // namespace pl
