/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "CubicIntersection_TestData.h"
#include <limits>

const Cubic pointDegenerates[] = {
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{1, 1}, {1, 1}, {1, 1}, {1, 1}},
    {{1 + PointEpsilon - std::numeric_limits<double>::epsilon(), 1},
     {1, 1 + PointEpsilon - std::numeric_limits<double>::epsilon()}, {1, 1}, {1, 1}},
    {{1 + PointEpsilon/2 - std::numeric_limits<double>::epsilon(), 1},
     {1 - (PointEpsilon/2 - std::numeric_limits<double>::epsilon()), 1}, {1, 1}, {1, 1}}
};

const size_t pointDegenerates_count = sizeof(pointDegenerates) / sizeof(pointDegenerates[0]);

const Cubic notPointDegenerates[] = {
    {{1 + PointEpsilon + std::numeric_limits<double>::epsilon(), 1}, {1, 1 + PointEpsilon}, {1, 1}, {1, 1}},
    {{1 + PointEpsilon/2 + std::numeric_limits<double>::epsilon(), 1}, {1 - PointEpsilon/2, 1}, {1, 1}, {1, 1}}
};

const size_t notPointDegenerates_count = sizeof(notPointDegenerates) / sizeof(notPointDegenerates[0]);

// from http://www.truetex.com/bezint.htm
const Cubic tests[][2] = {
    { // intersects in one place (data gives bezier clip fits
     {{0, 45},
      {6.0094158284751593, 51.610357411322688},
      {12.741093228940867, 55.981703949474607},
      {20.021417396476362, 58.652245509710262}},
     {{2.2070737699246674, 52.703494107327209},
      {31.591482272629477, 23.811002295222025},
      {76.824588616426425, 44.049473790502674},
      {119.25488947221436, 55.599248272955073}}
    },
    { // intersects in three places
        {{0, 45}, {50, 100}, {150,   0}, {200, 55}},
        {{0, 55}, {50,   0}, {150, 100}, {200, 45}}
    },
    { // intersects in one place, cross over is nearly parallel
        {{0,   0}, {0, 100}, {200,   0}, {200, 100}},
        {{0, 100}, {0,   0}, {200, 100}, {200,   0}}
    },
    { // intersects in two places
        {{0,   0}, {0, 100}, {200, 100}, {200,   0}},
        {{0, 100}, {0,   0}, {200,   0}, {200, 100}}
    },
    {
        {{150, 100}, {150 + 0.1, 150}, {150, 200}, {150, 250}},
        {{250, 150}, {200, 150 + 0.1}, {150, 150}, {100, 150}}
    },
    { // single intersection around 168,185
        {{200, 100}, {150, 100}, {150, 150}, {200, 150}},
        {{250, 150}, {250, 100}, {100, 100}, {100, 150}}
    },
    {
        {{1.0, 1.5}, {15.5, 0.5}, {-8.0, 3.5}, {5.0, 1.5}},
        {{4.0, 0.5}, {5.0, 15.0}, {2.0, -8.5}, {4.0, 4.5}}
    },
    {
        {{664.00168, 0},       {726.11545, 124.22757}, {736.89069, 267.89743}, {694.0017, 400.0002}},
        {{850.66843, 115.55563}, {728.515, 115.55563}, {725.21347, 275.15309}, {694.0017, 400.0002}}
    },
    {
        {{1, 1},   {12.5, 6.5}, {-4, 6.5}, {7.5, 1}},
        {{1, 6.5}, {12.5, 1},   {-4, 1},   {.5, 6}}
    },
    {
        {{315.748, 312.84}, {312.644, 318.134}, {305.836, 319.909}, {300.542, 316.804}},
        {{317.122, 309.05}, {316.112, 315.102}, {310.385, 319.19},  {304.332, 318.179}}
    },
    {
        {{1046.604051, 172.937967},  {1046.604051, 178.9763059}, {1041.76745,  183.9279165}, {1035.703842, 184.0432409}},
        {{1046.452235, 174.7640504}, {1045.544872, 180.1973817}, {1040.837966, 184.0469882}, {1035.505925, 184.0469882}}
    },
    {
        {{125.79356, 199.57382}, {51.16556, 128.93575}, {87.494,  16.67848}, {167.29361, 16.67848}},
        {{167.29361, 55.81876}, {100.36128, 55.81876}, {68.64099, 145.4755}, {125.7942, 199.57309}}
    }
};

const size_t tests_count = sizeof(tests) / sizeof(tests[0]);

Cubic hexTests[][2] = {
    {
        {{0}} // placeholder for hex converted below
    }
};

const size_t hexTests_count = sizeof(hexTests) / sizeof(hexTests[0]);

static const uint64_t testx[2][8] = {
    {
        0xf0d0d1ca63075a40LLU, 0x9408ce996a237740LLU, 0x6d5675460fbe5e40LLU, 0x6ef501e1b7487940LLU,
        0x9a71d2f8143d6540LLU, 0x6bc18bbe02907a40LLU, 0x5b94d92093aa6b40LLU, 0x6ac18bbe02907a40LLU
    },
    {
        0x92c56ed7b6145d40LLU, 0xede4f1255edb7740LLU, 0x1138c1101af75940LLU, 0x42e4f1255edb7740LLU,
        0x408e51603ad95640LLU, 0x1e2e8fe9dd927740LLU, 0x1cb4777cd3a75440LLU, 0x212e1390de017740LLU
    }
};

void convert_testx() {
    const uint64_t* inPtr = testx[0];
    double* outPtr = &hexTests[sizeof(tests) / sizeof(tests[0]) - 1][0][0].x;
    for (unsigned index = 0; index < sizeof(testx) / sizeof(testx[0][0]); ++index) {
        uint64_t input = *inPtr++;
        unsigned char* output = (unsigned char*) outPtr++;
        for (unsigned byte = 0; byte < sizeof(input); ++byte) {
            output[byte] = input >> (7 - byte) * 8;
        }
    }
}

const Cubic lines[] = {
    {{0, 0}, {0, 0}, {0, 0}, {1, 0}}, // 0: horizontal
    {{0, 0}, {0, 0}, {1, 0}, {0, 0}},
    {{0, 0}, {1, 0}, {0, 0}, {0, 0}},
    {{1, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{1, 0}, {2, 0}, {3, 0}, {4, 0}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 1}}, // 5: vertical
    {{0, 0}, {0, 0}, {0, 1}, {0, 0}},
    {{0, 0}, {0, 1}, {0, 0}, {0, 0}},
    {{0, 1}, {0, 0}, {0, 0}, {0, 0}},
    {{0, 1}, {0, 2}, {0, 3}, {0, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {1, 1}}, // 10: 3 coincident
    {{0, 0}, {0, 0}, {1, 1}, {0, 0}},
    {{0, 0}, {1, 1}, {0, 0}, {0, 0}},
    {{1, 1}, {0, 0}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {1, 1}, {2, 2}}, // 14: 2 coincident
    {{0, 0}, {1, 1}, {0, 0}, {2, 2}},
    {{0, 0}, {1, 1}, {2, 2}, {0, 0}},
    {{1, 1}, {0, 0}, {0, 0}, {2, 2}}, // 17:
    {{1, 1}, {0, 0}, {2, 2}, {0, 0}},
    {{1, 1}, {2, 2}, {0, 0}, {0, 0}},
    {{1, 1}, {2, 2}, {3, 3}, {2, 2}}, // middle-last coincident
    {{1, 1}, {2, 2}, {3, 3}, {3, 3}}, // middle-last coincident
    {{1, 1}, {1, 1}, {2, 2}, {2, 2}}, // 2 pairs coincident
    {{1, 1}, {2, 2}, {1, 1}, {2, 2}},
    {{1, 1}, {2, 2}, {2, 2}, {1, 1}},
    {{1, 1}, {1, 1}, {3, 3}, {3, 3}}, // first-middle middle-last coincident
    {{1, 1}, {2, 2}, {3, 3}, {4, 4}}, // no coincident
    {{1, 1}, {3, 3}, {2, 2}, {4, 4}},
    {{1, 1}, {2, 2}, {4, 4}, {3, 3}},
    {{1, 1}, {3, 3}, {4, 4}, {2, 2}},
    {{1, 1}, {4, 4}, {2, 2}, {3, 3}},
    {{1, 1}, {4, 4}, {3, 3}, {2, 2}},
    {{2, 2}, {1, 1}, {3, 3}, {4, 4}},
    {{2, 2}, {1, 1}, {4, 4}, {3, 3}},
    {{2, 2}, {3, 3}, {1, 1}, {4, 4}},
    {{2, 2}, {3, 3}, {4, 4}, {1, 1}},
    {{2, 2}, {4, 4}, {1, 1}, {3, 3}},
    {{2, 2}, {4, 4}, {3, 3}, {1, 1}},
};

const size_t lines_count = sizeof(lines) / sizeof(lines[0]);

// 'not a line' tries to fool the line detection code
const Cubic notLines[] = {
    {{0, 0}, {0, 0}, {0, 1}, {1, 0}},
    {{0, 0}, {0, 1}, {0, 0}, {1, 0}},
    {{0, 0}, {0, 1}, {1, 0}, {0, 0}},
    {{0, 1}, {0, 0}, {0, 0}, {1, 0}},
    {{0, 1}, {0, 0}, {1, 0}, {0, 0}},
    {{0, 1}, {1, 0}, {0, 0}, {0, 0}},
};

const size_t notLines_count = sizeof(notLines) / sizeof(notLines[0]);

static const double E = PointEpsilon * 2;
static const double F = PointEpsilon * 3;

const Cubic modEpsilonLines[] = {
    {{0, E}, {0, 0}, {0, 0}, {1, 0}}, // horizontal
    {{0, 0}, {0, E}, {1, 0}, {0, 0}},
    {{0, 0}, {1, 0}, {0, E}, {0, 0}},
    {{1, 0}, {0, 0}, {0, 0}, {0, E}},
    {{1, E}, {2, 0}, {3, 0}, {4, 0}},
    {{E, 0}, {0, 0}, {0, 0}, {0, 1}}, // vertical
    {{0, 0}, {E, 0}, {0, 1}, {0, 0}},
    {{0, 0}, {0, 1}, {E, 0}, {0, 0}},
    {{0, 1}, {0, 0}, {0, 0}, {E, 0}},
    {{E, 1}, {0, 2}, {0, 3}, {0, 4}},
    {{E, 0}, {0, 0}, {0, 0}, {1, 1}}, // 3 coincident
    {{0, 0}, {E, 0}, {1, 1}, {0, 0}},
    {{0, 0}, {1, 1}, {E, 0}, {0, 0}},
    {{1, 1}, {0, 0}, {0, 0}, {E, 0}},
    {{0, E}, {0, 0}, {1, 1}, {2, 2}}, // 2 coincident
    {{0, 0}, {1, 1}, {0, E}, {2, 2}},
    {{0, 0}, {1, 1}, {2, 2}, {0, E}},
    {{1, 1}, {0, E}, {0, 0}, {2, 2}},
    {{1, 1}, {0, E}, {2, 2}, {0, 0}},
    {{1, 1}, {2, 2}, {E, 0}, {0, 0}},
    {{1, 1}, {2, 2+E}, {3, 3}, {2, 2}}, // middle-last coincident
    {{1, 1}, {2+E, 2}, {3, 3}, {3, 3}}, // middle-last coincident
    {{1, 1}, {1, 1}, {2, 2}, {2+E, 2}}, // 2 pairs coincident
    {{1, 1}, {2, 2}, {1, 1}, {2+E, 2}},
    {{1, 1}, {2, 2}, {2, 2+E}, {1, 1}},
    {{1, 1}, {1, 1+E}, {3, 3}, {3, 3}}, // first-middle middle-last coincident
    {{1, 1}, {2+E, 2}, {3, 3}, {4, 4}}, // no coincident
    {{1, 1}, {3, 3}, {2, 2}, {4, 4+F}}, // INVESTIGATE: why the epsilon is bigger
    {{1, 1+F}, {2, 2}, {4, 4}, {3, 3}}, // INVESTIGATE: why the epsilon is bigger
    {{1, 1}, {3, 3}, {4, 4+E}, {2, 2}},
    {{1, 1}, {4, 4}, {2, 2}, {3, 3+E}},
    {{1, 1}, {4, 4}, {3, 3}, {2+E, 2}},
    {{2, 2}, {1, 1}, {3+E, 3}, {4, 4}},
    {{2, 2}, {1+E, 1}, {4, 4}, {3, 3}},
    {{2, 2+E}, {3, 3}, {1, 1}, {4, 4}},
    {{2+E, 2}, {3, 3}, {4, 4}, {1, 1}},
    {{2, 2}, {4+E, 4}, {1, 1}, {3, 3}},
    {{2, 2}, {4, 4}, {3, 3}, {1, 1+E}},
};

const size_t modEpsilonLines_count = sizeof(modEpsilonLines) / sizeof(modEpsilonLines[0]);

static const double D = PointEpsilon / 2;
static const double G = PointEpsilon / 3;

const Cubic lessEpsilonLines[] = {
    {{0, D}, {0, 0}, {0, 0}, {1, 0}}, // horizontal
    {{0, 0}, {0, D}, {1, 0}, {0, 0}},
    {{0, 0}, {1, 0}, {0, D}, {0, 0}},
    {{1, 0}, {0, 0}, {0, 0}, {0, D}},
    {{1, D}, {2, 0}, {3, 0}, {4, 0}},
    {{D, 0}, {0, 0}, {0, 0}, {0, 1}}, // vertical
    {{0, 0}, {D, 0}, {0, 1}, {0, 0}},
    {{0, 0}, {0, 1}, {D, 0}, {0, 0}},
    {{0, 1}, {0, 0}, {0, 0}, {D, 0}},
    {{D, 1}, {0, 2}, {0, 3}, {0, 4}},
    {{D, 0}, {0, 0}, {0, 0}, {1, 1}}, // 3 coincident
    {{0, 0}, {D, 0}, {1, 1}, {0, 0}},
    {{0, 0}, {1, 1}, {D, 0}, {0, 0}},
    {{1, 1}, {0, 0}, {0, 0}, {D, 0}},
    {{0, D}, {0, 0}, {1, 1}, {2, 2}}, // 2 coincident
    {{0, 0}, {1, 1}, {0, D}, {2, 2}},
    {{0, 0}, {1, 1}, {2, 2}, {0, D}},
    {{1, 1}, {0, D}, {0, 0}, {2, 2}},
    {{1, 1}, {0, D}, {2, 2}, {0, 0}},
    {{1, 1}, {2, 2}, {D, 0}, {0, 0}},
    {{1, 1}, {2, 2+D}, {3, 3}, {2, 2}}, // middle-last coincident
    {{1, 1}, {2+D, 2}, {3, 3}, {3, 3}}, // middle-last coincident
    {{1, 1}, {1, 1}, {2, 2}, {2+D, 2}}, // 2 pairs coincident
    {{1, 1}, {2, 2}, {1, 1}, {2+D, 2}},
    {{1, 1}, {2, 2}, {2, 2+D}, {1, 1}},
    {{1, 1}, {1, 1+D}, {3, 3}, {3, 3}}, // first-middle middle-last coincident
    {{1, 1}, {2+D/2, 2}, {3, 3}, {4, 4}}, // no coincident (FIXME: N as opposed to N/2 failed)
    {{1, 1}, {3, 3}, {2, 2}, {4, 4+D}},
    {{1, 1+D}, {2, 2}, {4, 4}, {3, 3}},
    {{1, 1}, {3, 3}, {4, 4+D}, {2, 2}},
    {{1, 1}, {4, 4}, {2, 2}, {3, 3+D}},
    {{1, 1}, {4, 4}, {3, 3}, {2+G, 2}}, // INVESTIGATE: why the epsilon is smaller
    {{2, 2}, {1, 1}, {3+D, 3}, {4, 4}},
    {{2, 2}, {1+D, 1}, {4, 4}, {3, 3}},
    {{2, 2+D}, {3, 3}, {1, 1}, {4, 4}},
    {{2+G, 2}, {3, 3}, {4, 4}, {1, 1}}, // INVESTIGATE: why the epsilon is smaller
    {{2, 2}, {4+D, 4}, {1, 1}, {3, 3}},
    {{2, 2}, {4, 4}, {3, 3}, {1, 1+D}},
};

const size_t lessEpsilonLines_count = sizeof(lessEpsilonLines) / sizeof(lessEpsilonLines[0]);

static const double N = -PointEpsilon / 2;
static const double M = -PointEpsilon / 3;

const Cubic negEpsilonLines[] = {
    {{0, N}, {0, 0}, {0, 0}, {1, 0}}, // horizontal
    {{0, 0}, {0, N}, {1, 0}, {0, 0}},
    {{0, 0}, {1, 0}, {0, N}, {0, 0}},
    {{1, 0}, {0, 0}, {0, 0}, {0, N}},
    {{1, N}, {2, 0}, {3, 0}, {4, 0}},
    {{N, 0}, {0, 0}, {0, 0}, {0, 1}}, // vertical
    {{0, 0}, {N, 0}, {0, 1}, {0, 0}},
    {{0, 0}, {0, 1}, {N, 0}, {0, 0}},
    {{0, 1}, {0, 0}, {0, 0}, {N, 0}},
    {{N, 1}, {0, 2}, {0, 3}, {0, 4}},
    {{N, 0}, {0, 0}, {0, 0}, {1, 1}}, // 3 coincident
    {{0, 0}, {N, 0}, {1, 1}, {0, 0}},
    {{0, 0}, {1, 1}, {N, 0}, {0, 0}},
    {{1, 1}, {0, 0}, {0, 0}, {N, 0}},
    {{0, N}, {0, 0}, {1, 1}, {2, 2}}, // 2 coincident
    {{0, 0}, {1, 1}, {0, N}, {2, 2}},
    {{0, 0}, {1, 1}, {2, 2}, {0, N}},
    {{1, 1}, {0, N}, {0, 0}, {2, 2}},
    {{1, 1}, {0, N}, {2, 2}, {0, 0}},
    {{1, 1}, {2, 2}, {N, 0}, {0, 0}},
    {{1, 1}, {2, 2+N}, {3, 3}, {2, 2}}, // middle-last coincident
    {{1, 1}, {2+N, 2}, {3, 3}, {3, 3}}, // middle-last coincident
    {{1, 1}, {1, 1}, {2, 2}, {2+N, 2}}, // 2 pairs coincident
    {{1, 1}, {2, 2}, {1, 1}, {2+N, 2}},
    {{1, 1}, {2, 2}, {2, 2+N}, {1, 1}},
    {{1, 1}, {1, 1+N}, {3, 3}, {3, 3}}, // first-middle middle-last coincident
    {{1, 1}, {2+N/2, 2}, {3, 3}, {4, 4}}, // no coincident (FIXME: N as opposed to N/2 failed)
    {{1, 1}, {3, 3}, {2, 2}, {4, 4+N}},
    {{1, 1+N}, {2, 2}, {4, 4}, {3, 3}},
    {{1, 1}, {3, 3}, {4, 4+N}, {2, 2}},
    {{1, 1}, {4, 4}, {2, 2}, {3, 3+N}},
    {{1, 1}, {4, 4}, {3, 3}, {2+M, 2}}, // INVESTIGATE: why the epsilon is smaller
    {{2, 2}, {1, 1}, {3+N, 3}, {4, 4}},
    {{2, 2}, {1+N, 1}, {4, 4}, {3, 3}},
    {{2, 2+N}, {3, 3}, {1, 1}, {4, 4}},
    {{2+M, 2}, {3, 3}, {4, 4}, {1, 1}}, // INVESTIGATE: why the epsilon is smaller
    {{2, 2}, {4+N, 4}, {1, 1}, {3, 3}},
    {{2, 2}, {4, 4}, {3, 3}, {1, 1+N}},
};

const size_t negEpsilonLines_count = sizeof(negEpsilonLines) / sizeof(negEpsilonLines[0]);
