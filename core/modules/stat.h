// Copyright (c) 2021, University of Southern California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// * Neither the names of the copyright holders nor the names of their
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef BESS_MODULES_STAT_H_
#define BESS_MODULES_STAT_H_


#include "../module.h"
#include "../pb/module_msg.pb.h"
#include "../utils/time.h"
#include "../utils/cuckoo_map.h"


using bess::utils::CuckooMap;

class Stat: public Module {
 public:
  static const Commands cmds;

  Stat()
      : Module(),
        frequency_(),
        lastTime_() {
    max_allowed_workers_ = Worker::kMaxWorkers;
  }

  CommandResponse Init(const bess::pb::StatArg &arg);
  CommandResponse GetInitialArg(const bess::pb::EmptyArg &);

  void ProcessBatch(Context *ctx, bess::PacketBatch *batch) override;

  std::string GetDesc() const override;

  CommandResponse CommandSetFrequency(const bess::pb::StatCommandSetFrequencyArg &arg);
  

 private:
  // 5 tuple id to identify a flow from a packet header information.
  struct FlowId {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint32_t src_port;
    uint32_t dst_port;
    uint8_t protocol;
  };
  
  // stores the metrics of the flow.
  struct Flow {
    FlowId id;                  // allows the flow to remove itself from the map
    uint64_t count_;
    uint64_t rate_;
    Flow() : id(), count_(), rate_() {};
    Flow(FlowId new_id)
        : id(new_id), count_(), rate_() {};
  };
  
  // hashes a FlowId
  struct Hash {
    // a similar method to boost's hash_combine in order to combine hashes
    inline void combine(std::size_t &hash, const unsigned int &val) const {
      std::hash<unsigned int> hasher;
      hash ^= hasher(val) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    bess::utils::HashResult operator()(const FlowId &id) const {
      std::size_t hash = 0;
      combine(hash, id.src_ip);
      combine(hash, id.dst_ip);
      combine(hash, id.src_port);
      combine(hash, id.dst_port);
      combine(hash, (uint32_t)id.protocol);
      return hash;
    }
  };
  struct EqualTo {
    bool operator()(const FlowId &id1, const FlowId &id2) const {
      bool ips = (id1.src_ip == id2.src_ip) && (id1.dst_ip == id2.dst_ip);
      bool ports =
          (id1.src_port == id2.src_port) && (id1.dst_port == id2.dst_port);
      return (ips && ports) && (id1.protocol == id2.protocol);
    }
  };

  //  Takes a Packet to get a flow id for. Returns the 5 element identifier for
  //  the flow that the packet belongs to
  FlowId GetId(bess::Packet *pkt);

  void ComputeRate();
  CommandResponse SetFrequency(uint64_t size);
  void DebugPrint() ;

  // Frequency at which the data sample needs to be collected.
  uint64_t frequency_;

  // Time for previous data point collection.
  uint64_t lastTime_;

  CuckooMap<FlowId, Flow *, Hash, EqualTo> flows_;
  bess::pb::StatArg init_arg_;

};


#endif  // BESS_MODULES_STAT_H_