// Copyright 2018 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QThread>

#include "UICommon/AutoUpdate.h"

// Refer to docs/autoupdate_overview.md for a detailed overview of the autoupdate process

class QWidget;

class Updater : public QThread, public AutoUpdateChecker
{
  Q_OBJECT
public:
  Updater(QWidget* parent, ShouldSilentlyFail silently_fail);

  void run() override;
  void OnErrorOccurred(CheckError error) override;
  void OnUpdateAvailable(const NewVersionInformation& info) override;

private:
  QWidget* m_parent;
  ShouldSilentlyFail m_silently_fail;
};
