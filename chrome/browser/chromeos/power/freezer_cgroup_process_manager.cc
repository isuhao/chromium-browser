// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/freezer_cgroup_process_manager.h"

#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "content/public/browser/browser_thread.h"

namespace chromeos {

namespace {
const char kFreezerPath[] = "/sys/fs/cgroup/freezer/chrome_renderers";
const char kToBeFrozen[] = "to_be_frozen";
const char kFreezerState[] = "freezer.state";
const char kCgroupProcs[] = "cgroup.procs";

const char kFreezeCommand[] = "FROZEN";
const char kThawCommand[] = "THAWED";

}  // namespace

class FreezerCgroupProcessManager::FileWorker {
 public:
  // Called on UI thread.
  explicit FileWorker(scoped_refptr<base::SequencedTaskRunner> file_thread)
      : ui_thread_(content::BrowserThread::GetMessageLoopProxyForThread(
            content::BrowserThread::UI)),
        file_thread_(file_thread) {
    DCHECK(ui_thread_->RunsTasksOnCurrentThread());
  }

  // Called on FILE thread.
  virtual ~FileWorker() { DCHECK(file_thread_->RunsTasksOnCurrentThread()); }

  void Start() {
    DCHECK(file_thread_->RunsTasksOnCurrentThread());

    default_control_path_ = base::FilePath(kFreezerPath).Append(kCgroupProcs);
    to_be_frozen_control_path_ = base::FilePath(kFreezerPath)
                                     .AppendASCII(kToBeFrozen)
                                     .AppendASCII(kCgroupProcs);
    to_be_frozen_state_path_ = base::FilePath(kFreezerPath)
                                   .AppendASCII(kToBeFrozen)
                                   .AppendASCII(kFreezerState);
    enabled_ = base::PathIsWritable(default_control_path_) &&
               base::PathIsWritable(to_be_frozen_control_path_) &&
               base::PathIsWritable(to_be_frozen_state_path_);

    if (!enabled_) {
      LOG(WARNING) << "Cgroup freezer does not exist or is not writable. "
                   << "Unable to freeze renderer processes.";
    }
  }

  void SetShouldFreezeRenderer(base::ProcessHandle handle, bool frozen) {
    DCHECK(file_thread_->RunsTasksOnCurrentThread());

    WriteCommandToFile(base::IntToString(handle),
                       frozen ? to_be_frozen_control_path_
                              : default_control_path_);
  }

  void FreezeRenderers() {
    DCHECK(file_thread_->RunsTasksOnCurrentThread());

    if (!enabled_) {
      LOG(ERROR) << "Attempting to freeze renderers when the freezer cgroup is "
                 << "not available.";
      return;
    }

    WriteCommandToFile(kFreezeCommand, to_be_frozen_state_path_);
  }

  void ThawRenderers(ResultCallback callback) {
    DCHECK(file_thread_->RunsTasksOnCurrentThread());

    if (!enabled_) {
      LOG(ERROR) << "Attempting to thaw renderers when the freezer cgroup is "
                 << "not available.";
      return;
    }

    bool result = WriteCommandToFile(kThawCommand, to_be_frozen_state_path_);
    ui_thread_->PostTask(FROM_HERE, base::Bind(callback, result));
  }

  void CheckCanFreezeRenderers(ResultCallback callback) {
    DCHECK(file_thread_->RunsTasksOnCurrentThread());

    ui_thread_->PostTask(FROM_HERE, base::Bind(callback, enabled_));
  }

 private:
  bool WriteCommandToFile(const std::string& command,
                          const base::FilePath& file) {
    int bytes = base::WriteFile(file, command.c_str(), command.size());
    if (bytes == -1) {
      PLOG(ERROR) << "Writing " << command << " to " << file.value()
                  << " failed";
      return false;
    } else if (bytes != static_cast<int>(command.size())) {
      LOG(ERROR) << "Only wrote " << bytes << " byte(s) when writing "
                 << command << " to " << file.value();
      return false;
    }
    return true;
  }

  scoped_refptr<base::SequencedTaskRunner> ui_thread_;
  scoped_refptr<base::SequencedTaskRunner> file_thread_;

  // Control path for the cgroup that is not frozen.
  base::FilePath default_control_path_;

  // Control and state paths for the cgroup whose processes will be frozen.
  base::FilePath to_be_frozen_control_path_;
  base::FilePath to_be_frozen_state_path_;

  bool enabled_;

  DISALLOW_COPY_AND_ASSIGN(FileWorker);
};

FreezerCgroupProcessManager::FreezerCgroupProcessManager()
    : file_thread_(content::BrowserThread::GetMessageLoopProxyForThread(
          content::BrowserThread::FILE)),
      file_worker_(new FileWorker(file_thread_)) {
  file_thread_->PostTask(FROM_HERE,
                         base::Bind(&FileWorker::Start,
                                    base::Unretained(file_worker_.get())));
}

FreezerCgroupProcessManager::~FreezerCgroupProcessManager() {
  file_thread_->DeleteSoon(FROM_HERE, file_worker_.release());
}

void FreezerCgroupProcessManager::SetShouldFreezeRenderer(
    base::ProcessHandle handle,
    bool frozen) {
  file_thread_->PostTask(FROM_HERE,
                         base::Bind(&FileWorker::SetShouldFreezeRenderer,
                                    base::Unretained(file_worker_.get()),
                                    handle, frozen));
}

void FreezerCgroupProcessManager::FreezeRenderers() {
  file_thread_->PostTask(FROM_HERE,
                         base::Bind(&FileWorker::FreezeRenderers,
                                    base::Unretained(file_worker_.get())));
}

void FreezerCgroupProcessManager::ThawRenderers(ResultCallback callback) {
  file_thread_->PostTask(FROM_HERE,
                         base::Bind(&FileWorker::ThawRenderers,
                                    base::Unretained(file_worker_.get()),
                                    callback));
}

void FreezerCgroupProcessManager::CheckCanFreezeRenderers(
    ResultCallback callback) {
  file_thread_->PostTask(FROM_HERE,
                         base::Bind(&FileWorker::CheckCanFreezeRenderers,
                                    base::Unretained(file_worker_.get()),
                                    callback));
}

}  // namespace chromeos
