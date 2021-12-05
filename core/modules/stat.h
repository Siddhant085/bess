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

class Stat: public Module {
 public:
  static const Commands cmds;

  Stat()
      : Module(),
        frequency_(),
        lastTime_(),
        count_(),
        rate_() {
//    is_task_ = false;
//    propagate_workers_ = false;
    max_allowed_workers_ = Worker::kMaxWorkers;
  }

  CommandResponse Init(const bess::pb::StatArg &arg);
  CommandResponse GetInitialArg(const bess::pb::EmptyArg &);

/*
  struct task_result RunTask(Context *ctx, bess::PacketBatch *batch,
                             void *arg) override;
                             */
  void ProcessBatch(Context *ctx, bess::PacketBatch *batch) override;

  std::string GetDesc() const override;

  CommandResponse CommandSetFrequency(const bess::pb::StatCommandSetFrequencyArg &arg);

 private:
  CommandResponse SetFrequency(uint64_t size);

  // Frequency at which the data sample needs to be collected.
  uint64_t frequency_;

  // Time for previous data point collection.
  uint64_t lastTime_;

  // Total number of packets processed since previous data point collection.
  uint64_t count_;

  // Rate of packet arrival.
  uint64_t rate_;

  bess::pb::StatArg init_arg_;

};


#endif  // BESS_MODULES_STAT_H_