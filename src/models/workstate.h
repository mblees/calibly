// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QObject>
#include <QQmlEngine>

#include "work.h"

// The state of a work on its cover, for QML (import Calibly.Models):
//   WorkState.Normal, WorkState.New, WorkState.Finished, WorkState.FileMissing
// Same values as Work::State; the models report it in the role "workState".
namespace WorkState {
Q_NAMESPACE
QML_ELEMENT

enum State {
    Normal = int(Work::State::Normal),
    New = int(Work::State::New),                 // never opened since it was added
    Finished = int(Work::State::Finished),       // a format was read / listened to the end
    FileMissing = int(Work::State::FileMissing), // a file was not found
};
Q_ENUM_NS(State)

} // namespace WorkState
