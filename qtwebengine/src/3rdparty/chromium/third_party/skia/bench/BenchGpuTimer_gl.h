
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef SkBenchGpuTimer_DEFINED
#define SkBenchGpuTimer_DEFINED

class SkGLContextHelper;

class BenchGpuTimer {
public:
    BenchGpuTimer(const SkGLContextHelper* glctx);
    ~BenchGpuTimer();
    void startGpu();
    double endGpu();
private:
    unsigned fQuery;
    int fStarted;
    const SkGLContextHelper* fContext;
    bool fSupported;
};

#endif
