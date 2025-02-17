
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "Benchmark.h"
#include "SkRandom.h"
#include "SkRegion.h"
#include "SkString.h"

static bool union_proc(SkRegion& a, SkRegion& b) {
    SkRegion result;
    return result.op(a, b, SkRegion::kUnion_Op);
}

static bool sect_proc(SkRegion& a, SkRegion& b) {
    SkRegion result;
    return result.op(a, b, SkRegion::kIntersect_Op);
}

static bool diff_proc(SkRegion& a, SkRegion& b) {
    SkRegion result;
    return result.op(a, b, SkRegion::kDifference_Op);
}

static bool diffrect_proc(SkRegion& a, SkRegion& b) {
    SkRegion result;
    return result.op(a, b.getBounds(), SkRegion::kDifference_Op);
}

static bool diffrectbig_proc(SkRegion& a, SkRegion& b) {
    SkRegion result;
    return result.op(a, a.getBounds(), SkRegion::kDifference_Op);
}

static bool containsrect_proc(SkRegion& a, SkRegion& b) {
    SkIRect r = a.getBounds();
    r.inset(r.width()/4, r.height()/4);
    (void)a.contains(r);

    r = b.getBounds();
    r.inset(r.width()/4, r.height()/4);
    return b.contains(r);
}

static bool sectsrgn_proc(SkRegion& a, SkRegion& b) {
    return a.intersects(b);
}

static bool sectsrect_proc(SkRegion& a, SkRegion& b) {
    SkIRect r = a.getBounds();
    r.inset(r.width()/4, r.height()/4);
    return a.intersects(r);
}

static bool containsxy_proc(SkRegion& a, SkRegion& b) {
    const SkIRect& r = a.getBounds();
    const int dx = r.width() / 8;
    const int dy = r.height() / 8;
    for (int y = r.fTop; y < r.fBottom; y += dy) {
        for (int x = r.fLeft; x < r.fRight; x += dx) {
            (void)a.contains(x, y);
        }
    }
    return true;
}

class RegionBench : public Benchmark {
public:
    typedef bool (*Proc)(SkRegion& a, SkRegion& b);

    SkRegion fA, fB;
    Proc     fProc;
    SkString fName;

    enum {
        W = 1024,
        H = 768,
    };

    SkIRect randrect(SkRandom& rand) {
        int x = rand.nextU() % W;
        int y = rand.nextU() % H;
        int w = rand.nextU() % W;
        int h = rand.nextU() % H;
        return SkIRect::MakeXYWH(x, y, w >> 1, h >> 1);
    }

    RegionBench(int count, Proc proc, const char name[])  {
        fProc = proc;
        fName.printf("region_%s_%d", name, count);

        SkRandom rand;
        for (int i = 0; i < count; i++) {
            fA.op(randrect(rand), SkRegion::kXOR_Op);
            fB.op(randrect(rand), SkRegion::kXOR_Op);
        }
    }

    virtual bool isSuitableFor(Backend backend) SK_OVERRIDE {
        return backend == kNonRendering_Backend;
    }

protected:
    virtual const char* onGetName() { return fName.c_str(); }

    virtual void onDraw(const int loops, SkCanvas* canvas) {
        Proc proc = fProc;
        for (int i = 0; i < loops; ++i) {
            proc(fA, fB);
        }
    }

private:
    typedef Benchmark INHERITED;
};

class RectSectBench : public Benchmark {
    enum {
        N = 1000
    };
    SkRect fArray0[N];
    SkRect fArray1[N];
    SkString fName;
    bool fNewWay;

public:
    static void RandRect(SkRect* r, SkRandom& rand) {
        r->set(rand.nextSScalar1(), rand.nextSScalar1(),
               rand.nextSScalar1(), rand.nextSScalar1());
        r->sort();
    }

    RectSectBench(bool newWay) : fNewWay(newWay) {
        fName.printf("rect_intersect_%s", newWay ? "new" : "old");

        SkRandom rand;
        for (int i = 0; i < N; i++) {
            RandRect(&fArray0[i], rand);
            RandRect(&fArray1[i], rand);
        }
    }

    virtual bool isSuitableFor(Backend backend) SK_OVERRIDE {
        return backend == kNonRendering_Backend;
    }

protected:
    virtual const char* onGetName() { return fName.c_str(); }

    virtual void onDraw(const int loops, SkCanvas* canvas) {
        for (int i = 0; i < loops; ++i) {
            if (fNewWay) {
                for (int j = 0; j < N; ++j) {
                    SkRect r = fArray0[j];
                    r.intersect2(fArray1[j]);
                }
            } else {
                for (int j = 0; j < N; ++j) {
                    SkRect r = fArray0[j];
                    r.intersect(fArray1[j]);
                }
            }
        }
    }

private:
    typedef Benchmark INHERITED;
};

///////////////////////////////////////////////////////////////////////////////

#define SMALL   16

DEF_BENCH( return SkNEW_ARGS(RegionBench, (SMALL, union_proc, "union")); )
DEF_BENCH( return SkNEW_ARGS(RegionBench, (SMALL, sect_proc, "intersect")); )
DEF_BENCH( return SkNEW_ARGS(RegionBench, (SMALL, diff_proc, "difference")); )
DEF_BENCH( return SkNEW_ARGS(RegionBench, (SMALL, diffrect_proc, "differencerect")); )
DEF_BENCH( return SkNEW_ARGS(RegionBench, (SMALL, diffrectbig_proc, "differencerectbig")); )
DEF_BENCH( return SkNEW_ARGS(RegionBench, (SMALL, containsrect_proc, "containsrect")); )
DEF_BENCH( return SkNEW_ARGS(RegionBench, (SMALL, sectsrgn_proc, "intersectsrgn")); )
DEF_BENCH( return SkNEW_ARGS(RegionBench, (SMALL, sectsrect_proc, "intersectsrect")); )
DEF_BENCH( return SkNEW_ARGS(RegionBench, (SMALL, containsxy_proc, "containsxy")); )

DEF_BENCH( return SkNEW_ARGS(RectSectBench, (false)); )
DEF_BENCH( return SkNEW_ARGS(RectSectBench, (true)); )
