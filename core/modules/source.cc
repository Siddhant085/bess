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

#include "source.h"


const Commands Source::cmds = {
    {"set_pkt_size", "SourceCommandSetPktSizeArg",
     MODULE_CMD_FUNC(&Source::CommandSetPktSize), Command::THREAD_SAFE},
    {"set_burst", "SourceCommandSetBurstArg",
     MODULE_CMD_FUNC(&Source::CommandSetBurst), Command::THREAD_SAFE},
    {"set_burst_size", "SourceCommandSetBurstSizeArg",
     MODULE_CMD_FUNC(&Source::CommandSetBurstSize), Command::THREAD_SAFE},
};

CommandResponse Source::Init(const bess::pb::SourceArg &arg) {
  CommandResponse err;

  task_id_t tid = RegisterTask(nullptr);
  if (tid == INVALID_TASK_ID)
    return CommandFailure(ENOMEM, "Task creation failed");

  pkt_size_ = 60;
  burst_ = bess::PacketBatch::kMaxBurst;

  if (arg.pkt_size() > 0) {
    if (arg.pkt_size() > SNBUF_DATA) {
      return CommandFailure(EINVAL, "Invalid packet size");
    }
    pkt_size_ = arg.pkt_size();
  }

  burst_ = bess::PacketBatch::kMaxBurst;
  burst_size_ = -1; // TODO: Remove constants

  return CommandSuccess();
}

CommandResponse Source::CommandSetBurst(
    const bess::pb::SourceCommandSetBurstArg &arg) {
  if (arg.burst() > bess::PacketBatch::kMaxBurst) {
    return CommandFailure(EINVAL, "burst size must be [0,%zu]",
                          bess::PacketBatch::kMaxBurst);
  }
  burst_ = arg.burst();

  return CommandSuccess();
}

CommandResponse Source::CommandSetPktSize(
    const bess::pb::SourceCommandSetPktSizeArg &arg) {
  uint64_t val = arg.pkt_size();
  if (val == 0 || val > SNBUF_DATA) {
    return CommandFailure(EINVAL, "Invalid packet size");
  }
  pkt_size_ = val;
  return CommandSuccess();
}

CommandResponse Source::CommandSetBurstSize(
    const bess::pb::SourceCommandSetBurstSizeArg &arg) {
  uint64_t val = arg.burst_size();
  burst_size_ = val + (uint64_t)get_epoch_time();
  return CommandSuccess();
}

struct task_result Source::RunTask(Context *ctx, bess::PacketBatch *batch,
                                   void *) {
  if (children_overload_ > 0) {
    return {.block = true, .packets = 0, .bits = 0};
  }

  const int pkt_overhead = 24;
  const int pkt_size = ACCESS_ONCE(pkt_size_);
  const uint32_t burst = ACCESS_ONCE(burst_);

  if (burst_size_ < (uint64_t)get_epoch_time()) {
    return {.block = true, .packets = 0, .bits = 0};
  }

  if (current_worker.packet_pool()->AllocBulk(batch->pkts(), burst, pkt_size)) {
    batch->set_cnt(burst);
    RunNextModule(ctx, batch);  // it's fine to call this function with cnt==0
    return {.block = false,
            .packets = burst,
            .bits = (pkt_size + pkt_overhead) * burst * 8};
  }

  return {.block = true, .packets = 0, .bits = 0};
}

ADD_MODULE(Source, "source",
           "infinitely generates packets with uninitialized data")
