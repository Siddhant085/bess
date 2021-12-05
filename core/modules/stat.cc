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

#define DEFAULT_STAT_FREQUENCY 1
#define SEC_IN_NS 1000000000
#define HZ_TO_NS(freq) (SEC_IN_NS/freq)

const Commands Stat::cmds = {
    {"set_frequency", "StatCommandSetFrequencyArg",
     MODULE_CMD_FUNC(&Stat::CommandSetFrequency), Command::THREAD_SAFE}};


CommandResponse Stat::Init(const bess::pb::StatArg &arg) {
//  task_id_t tid;
  CommandResponse err;
  uint64_t freq = DEFAULT_STAT_FREQUENCY;

/*
  tid = RegisterTask(nullptr);
  if (tid == INVALID_TASK_ID) {
    return CommandFailure(ENOMEM, "Task creation failed");
  }
*/

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
  return bess::utils::Format("%lu", rate_);
}

/* from upstream */
void Stat::ProcessBatch(Context *ctx, bess::PacketBatch *batch) {
  uint64_t now_ns = tsc_to_ns(rdtsc()); 
  if (now_ns < lastTime_ + HZ_TO_NS(frequency_)) {
    count_ += batch->cnt();
  } else {
    rate_ = count_ *(SEC_IN_NS/HZ_TO_NS(frequency_));
    count_ = 0;
    lastTime_ = tsc_to_ns(rdtsc());
  }
  RunNextModule(ctx, batch);
}

/* to downstream */
/*
struct task_result Stat::RunTask(Context *ctx, bess::PacketBatch *batch,
                                  void *) {

  LOG(ERROR) << "Run Task Stat module";
  RunNextModule(ctx, batch);

  return {.block = false,
          .packets = 0,
          .bits = 0};
}
*/


CommandResponse Stat::SetFrequency(uint64_t freq) {
  // TODO: Add any checks and validations.
  frequency_ = freq;
  return CommandSuccess();
}

CommandResponse Stat::CommandSetFrequency(
    const bess::pb::StatCommandSetFrequencyArg &arg) {
  return SetFrequency(arg.frequency());
}

ADD_MODULE(Stat, "stat",
           "Calculates statistics.")
