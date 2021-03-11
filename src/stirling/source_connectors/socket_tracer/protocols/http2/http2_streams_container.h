#pragma once

#include <deque>
#include <string>

#include "src/common/base/mixins.h"
#include "src/stirling/source_connectors/socket_tracer/protocols/http2/types.h"

namespace pl {
namespace stirling {

/**
 * HTTP2StreamsContainer is an object that holds the captured HTTP2 stream data from BPF.
 * This is managed differently from other protocols because it comes as UProbe data
 * and is already structured. This in contrast to other protocols which are captured via
 * KProbes and need to be parsed.
 */
class HTTP2StreamsContainer : NotCopyMoveable {
 public:
  const std::deque<protocols::http2::Stream>& streams() const { return streams_; }
  std::deque<protocols::http2::Stream>* mutable_streams() { return &streams_; }

  /**
   * Get the HTTP2 stream for the given stream ID and the direction of traffic.
   * @param write_event==true for send HalfStream, write_event==false for recv HalfStream.
   */
  protocols::http2::HalfStream* HalfStreamPtr(uint32_t stream_id, bool write_event);

  /**
   * Cleans up the HTTP2 events from BPF uprobes that are too old,
   * either because they are too far back in time, or too far back in bytes.
   */
  void Cleanup(size_t size_limit_bytes, int expiration_duration_secs);

  /**
   * Erase n stream IDs from the head of the streams container.
   */
  void EraseHead(size_t n);

  std::string DebugString(std::string_view prefix) const {
    std::string info;
    info += absl::Substitute("$0active streams=$1\n", prefix, streams_.size());
    return info;
  }

 private:
  std::deque<protocols::http2::Stream> streams_;

  // The oldest active Stream ID. Used to managed the size of the streams_ deque.
  uint32_t oldest_active_stream_id_;
};

}  // namespace stirling
}  // namespace pl