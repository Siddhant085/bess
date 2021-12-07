// Copyright (c) 2014-2016, The Regents of the University of California.
// Copyright (c) 2016-2017, Nefeli Networks, Inc.
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

#include "stat.h"

#include <cstdlib>

#include "../utils/format.h"
#include "../utils/ether.h"
#include "../utils/ip.h"
#include "../utils/udp.h"

#define DEFAULT_STAT_FREQUENCY 1
#define SEC_IN_NS 1000000000
#define HZ_TO_NS(freq) (SEC_IN_NS/freq)
#define THRESHOLD 499900

const Commands Stat::cmds = {
    {"set_frequency", "StatCommandSetFrequencyArg",
     MODULE_CMD_FUNC(&Stat::CommandSetFrequency), Command::THREAD_SAFE}};


CommandResponse Stat::Init(const bess::pb::StatArg &arg) {
  CommandResponse err;
  uint64_t freq = DEFAULT_STAT_FREQUENCY;

  if (arg.frequency() != 0) {
      freq = arg.frequency();
  }

  err = SetFrequency(freq);
  if (err.error().code() != 0) {
    return err;
  }

  lastTime_ = tsc_to_ns(rdtsc());

  init_arg_ = arg;
  return CommandSuccess();
}

CommandResponse Stat::GetInitialArg(const bess::pb::EmptyArg &) {
  return CommandSuccess(init_arg_);
}

std::string Stat::GetDesc() const {
  return bess::utils::Format("%lu", frequency_);
}

Stat::FlowId Stat::GetId(bess::Packet *pkt) {
  using bess::utils::Ethernet;
  using bess::utils::Ipv4;
  using bess::utils::Udp;

  Ethernet *eth = pkt->head_data<Ethernet *>();
  Ipv4 *ip = reinterpret_cast<Ipv4 *>(eth + 1);
  size_t ip_bytes = ip->header_length << 2;
  Udp *udp = reinterpret_cast<Udp *>(reinterpret_cast<uint8_t *>(ip) +
                                     ip_bytes);  // Assumes a l-4 header
  // TODO(joshua): handle packet fragmentation
  FlowId id = {ip->src.value(), ip->dst.value(), udp->src_port.value(),
               udp->dst_port.value(), ip->protocol};
  return id;
}

void Stat::ComputeRate() {
  uint64_t totalFlow = 0;
  uint64_t maxFlowRate = 0;
  Flow *maxFlow = nullptr;
  for (auto ptr = flows_.begin(); ptr != flows_.end(); ptr++) {
    Flow *f = ptr->second;
    f->rate_ = f->count_ *(SEC_IN_NS/HZ_TO_NS(frequency_));
    totalFlow += f->rate_;
    f->count_ = 0;
    if (f->rate_ > maxFlowRate){
      maxFlowRate = f->rate_;
      maxFlow = f;
    }
  }
  lastTime_ = tsc_to_ns(rdtsc());
  if (totalFlow>THRESHOLD) {
    LOG(ERROR) << "Threshold exceeded. biggest flow rate: " << maxFlowRate;
    LOG(ERROR) << "Total flow" << totalFlow;
    LOG(ERROR) << "MaxFlow: " << maxFlow->id.src_ip;
  }
//  DebugPrint();
}

/* from upstream */
void Stat::ProcessBatch(Context *ctx, bess::PacketBatch *batch) {
  // insert packets in the batch into their corresponding flows
  int cnt = batch->cnt();
  for (int i = 0; i < cnt; i++) {
    bess::Packet *pkt = batch->pkts()[i];

    // TODO(joshua): Add support for fragmented packets.
    FlowId id = GetId(pkt);
    auto it = flows_.Find(id);

    // if the Flow doesn't exist create one
    // and add the packet to the new Flow
    if (it == nullptr) {    
      Flow *f = new Flow(id);
      f->count_ = 1;
      flows_.Insert(id, f);
    } else {
      it->second->count_ += 1;
    }
  }
  uint64_t now_ns = tsc_to_ns(rdtsc()); 
  if (now_ns >= lastTime_ + HZ_TO_NS(frequency_)) {
    ComputeRate();
  }
  RunNextModule(ctx, batch);
}

CommandResponse Stat::SetFrequency(uint64_t freq) {
  // TODO: Add any checks and validations.
  frequency_ = freq;
  return CommandSuccess();
}

CommandResponse Stat::CommandSetFrequency(
    const bess::pb::StatCommandSetFrequencyArg &arg) {
  return SetFrequency(arg.frequency());
}

void Stat::DebugPrint(){
  uint64_t maxRate = 0;
  Flow *maxFlow = nullptr;
  for (auto ptr = flows_.begin(); ptr != flows_.end(); ptr++) {
    Flow *f = ptr->second;
    if (f->rate_>maxRate) {
      maxRate = f->rate_;
      maxFlow = f;
    }
  }
  LOG(ERROR) << "MaxRate: " << maxRate ;
  if (maxFlow != nullptr) {
    LOG(ERROR) << "MaxFlow: " << maxFlow->id.src_ip;
  }
  LOG(ERROR) << "Count: " << flows_.Count();
}

ADD_MODULE(Stat, "stat",
           "Calculates statistics.")
