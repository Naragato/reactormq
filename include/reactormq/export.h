//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#ifndef REACTORMQ_API

#ifdef REACTORMQ_UNREAL_API
#define REACTORMQ_API REACTORMQ_UNREAL_API

#elif defined(REACTORMQ_BUILD_STATIC)
#define REACTORMQ_API

#elif defined(_WIN32) || defined(_WIN64)
#ifdef REACTORMQ_BUILD_SHARED
#define REACTORMQ_API __declspec(dllexport)
#else
#define REACTORMQ_API __declspec(dllimport)
#endif // REACTORMQ_BUILD_SHARED

#else
#if defined(__GNUC__) && (__GNUC__ >= 4)
#define REACTORMQ_API __attribute__((visibility("default")))
#else
#define REACTORMQ_API
#endif
#endif // REACTORMQ_UNREAL_API

#endif // REACTORMQ_API