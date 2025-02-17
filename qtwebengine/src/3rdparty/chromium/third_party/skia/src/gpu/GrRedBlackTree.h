/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrRedBlackTree_DEFINED
#define GrRedBlackTree_DEFINED

#include "GrConfig.h"
#include "SkTypes.h"

template <typename T>
class GrLess {
public:
    bool operator()(const T& a, const T& b) const { return a < b; }
};

template <typename T>
class GrLess<T*> {
public:
    bool operator()(const T* a, const T* b) const { return *a < *b; }
};

class GrStrLess {
public:
    bool operator()(const char* a, const char* b) const { return strcmp(a,b) < 0; }
};

/**
 * In debug build this will cause full traversals of the tree when the validate
 * is called on insert and remove. Useful for debugging but very slow.
 */
#define DEEP_VALIDATE 0

/**
 * A sorted tree that uses the red-black tree algorithm. Allows duplicate
 * entries. Data is of type T and is compared using functor C. A single C object
 * will be created and used for all comparisons.
 */
template <typename T, typename C = GrLess<T> >
class GrRedBlackTree : SkNoncopyable {
public:
    /**
     * Creates an empty tree.
     */
    GrRedBlackTree();
    virtual ~GrRedBlackTree();

    /**
     * Class used to iterater through the tree. The valid range of the tree
     * is given by [begin(), end()). It is legal to dereference begin() but not
     * end(). The iterator has preincrement and predecrement operators, it is
     * legal to decerement end() if the tree is not empty to get the last
     * element. However, a last() helper is provided.
     */
    class Iter;

    /**
     * Add an element to the tree. Duplicates are allowed.
     * @param t     the item to add.
     * @return  an iterator to the item.
     */
    Iter insert(const T& t);

    /**
     * Removes all items in the tree.
     */
    void reset();

    /**
     * @return true if there are no items in the tree, false otherwise.
     */
    bool empty() const {return 0 == fCount;}

    /**
     * @return the number of items in the tree.
     */
    int  count() const {return fCount;}

    /**
     * @return  an iterator to the first item in sorted order, or end() if empty
     */
    Iter begin();
    /**
     * Gets the last valid iterator. This is always valid, even on an empty.
     * However, it can never be dereferenced. Useful as a loop terminator.
     * @return  an iterator that is just beyond the last item in sorted order.
     */
    Iter end();
    /**
     * @return  an iterator that to the last item in sorted order, or end() if
     * empty.
     */
    Iter last();

    /**
     * Finds an occurrence of an item.
     * @param t     the item to find.
     * @return an iterator to a tree element equal to t or end() if none exists.
     */
    Iter find(const T& t);
    /**
     * Finds the first of an item in iterator order.
     * @param t     the item to find.
     * @return  an iterator to the first element equal to t or end() if
     *          none exists.
     */
    Iter findFirst(const T& t);
    /**
     * Finds the last of an item in iterator order.
     * @param t     the item to find.
     * @return  an iterator to the last element equal to t or end() if
     *          none exists.
     */
    Iter findLast(const T& t);
    /**
     * Gets the number of items in the tree equal to t.
     * @param t     the item to count.
     * @return  number of items equal to t in the tree
     */
    int countOf(const T& t) const;

    /**
     * Removes the item indicated by an iterator. The iterator will not be valid
     * afterwards.
     *
     * @param iter      iterator of item to remove. Must be valid (not end()).
     */
    void remove(const Iter& iter) { deleteAtNode(iter.fN); }

private:
    enum Color {
        kRed_Color,
        kBlack_Color
    };

    enum Child {
        kLeft_Child  = 0,
        kRight_Child = 1
    };

    struct Node {
        T       fItem;
        Color   fColor;

        Node*   fParent;
        Node*   fChildren[2];
    };

    void rotateRight(Node* n);
    void rotateLeft(Node* n);

    static Node* SuccessorNode(Node* x);
    static Node* PredecessorNode(Node* x);

    void deleteAtNode(Node* x);
    static void RecursiveDelete(Node* x);

    int onCountOf(const Node* n, const T& t) const;

#ifdef SK_DEBUG
    void validate() const;
    int checkNode(Node* n, int* blackHeight) const;
    // checks relationship between a node and its children. allowRedRed means
    // node may be in an intermediate state where a red parent has a red child.
    bool validateChildRelations(const Node* n, bool allowRedRed) const;
    // place to stick break point if validateChildRelations is failing.
    bool validateChildRelationsFailed() const { return false; }
#else
    void validate() const {}
#endif

    int     fCount;
    Node*   fRoot;
    Node*   fFirst;
    Node*   fLast;

    const C fComp;
};

template <typename T, typename C>
class GrRedBlackTree<T,C>::Iter {
public:
    Iter() {};
    Iter(const Iter& i) {fN = i.fN; fTree = i.fTree;}
    Iter& operator =(const Iter& i) {
        fN = i.fN;
        fTree = i.fTree;
        return *this;
    }
    // altering the sort value of the item using this method will cause
    // errors.
    T& operator *() const { return fN->fItem; }
    bool operator ==(const Iter& i) const {
        return fN == i.fN && fTree == i.fTree;
    }
    bool operator !=(const Iter& i) const { return !(*this == i); }
    Iter& operator ++() {
        SkASSERT(*this != fTree->end());
        fN = SuccessorNode(fN);
        return *this;
    }
    Iter& operator --() {
        SkASSERT(*this != fTree->begin());
        if (NULL != fN) {
            fN = PredecessorNode(fN);
        } else {
            *this = fTree->last();
        }
        return *this;
    }

private:
    friend class GrRedBlackTree;
    explicit Iter(Node* n, GrRedBlackTree* tree) {
        fN = n;
        fTree = tree;
    }
    Node* fN;
    GrRedBlackTree* fTree;
};

template <typename T, typename C>
GrRedBlackTree<T,C>::GrRedBlackTree() : fComp() {
    fRoot = NULL;
    fFirst = NULL;
    fLast = NULL;
    fCount = 0;
    validate();
}

template <typename T, typename C>
GrRedBlackTree<T,C>::~GrRedBlackTree() {
    RecursiveDelete(fRoot);
}

template <typename T, typename C>
typename GrRedBlackTree<T,C>::Iter GrRedBlackTree<T,C>::begin() {
    return Iter(fFirst, this);
}

template <typename T, typename C>
typename GrRedBlackTree<T,C>::Iter GrRedBlackTree<T,C>::end() {
    return Iter(NULL, this);
}

template <typename T, typename C>
typename GrRedBlackTree<T,C>::Iter GrRedBlackTree<T,C>::last() {
    return Iter(fLast, this);
}

template <typename T, typename C>
typename GrRedBlackTree<T,C>::Iter GrRedBlackTree<T,C>::find(const T& t) {
    Node* n = fRoot;
    while (NULL != n) {
        if (fComp(t, n->fItem)) {
            n = n->fChildren[kLeft_Child];
        } else {
            if (!fComp(n->fItem, t)) {
                return Iter(n, this);
            }
            n = n->fChildren[kRight_Child];
        }
    }
    return end();
}

template <typename T, typename C>
typename GrRedBlackTree<T,C>::Iter GrRedBlackTree<T,C>::findFirst(const T& t) {
    Node* n = fRoot;
    Node* leftMost = NULL;
    while (NULL != n) {
        if (fComp(t, n->fItem)) {
            n = n->fChildren[kLeft_Child];
        } else {
            if (!fComp(n->fItem, t)) {
                // found one. check if another in left subtree.
                leftMost = n;
                n = n->fChildren[kLeft_Child];
            } else {
                n = n->fChildren[kRight_Child];
            }
        }
    }
    return Iter(leftMost, this);
}

template <typename T, typename C>
typename GrRedBlackTree<T,C>::Iter GrRedBlackTree<T,C>::findLast(const T& t) {
    Node* n = fRoot;
    Node* rightMost = NULL;
    while (NULL != n) {
        if (fComp(t, n->fItem)) {
            n = n->fChildren[kLeft_Child];
        } else {
            if (!fComp(n->fItem, t)) {
                // found one. check if another in right subtree.
                rightMost = n;
            }
            n = n->fChildren[kRight_Child];
        }
    }
    return Iter(rightMost, this);
}

template <typename T, typename C>
int GrRedBlackTree<T,C>::countOf(const T& t) const {
    return onCountOf(fRoot, t);
}

template <typename T, typename C>
int GrRedBlackTree<T,C>::onCountOf(const Node* n, const T& t) const {
    // this is count*log(n) :(
    while (NULL != n) {
        if (fComp(t, n->fItem)) {
            n = n->fChildren[kLeft_Child];
        } else {
            if (!fComp(n->fItem, t)) {
                int count = 1;
                count += onCountOf(n->fChildren[kLeft_Child], t);
                count += onCountOf(n->fChildren[kRight_Child], t);
                return count;
            }
            n = n->fChildren[kRight_Child];
        }
    }
    return 0;

}

template <typename T, typename C>
void GrRedBlackTree<T,C>::reset() {
    RecursiveDelete(fRoot);
    fRoot = NULL;
    fFirst = NULL;
    fLast = NULL;
    fCount = 0;
}

template <typename T, typename C>
typename GrRedBlackTree<T,C>::Iter GrRedBlackTree<T,C>::insert(const T& t) {
    validate();

    ++fCount;

    Node* x = SkNEW(Node);
    x->fChildren[kLeft_Child] = NULL;
    x->fChildren[kRight_Child] = NULL;
    x->fItem = t;

    Node* returnNode = x;

    Node* gp = NULL;
    Node* p = NULL;
    Node* n = fRoot;
    Child pc = kLeft_Child; // suppress uninit warning
    Child gpc = kLeft_Child;

    bool first = true;
    bool last = true;
    while (NULL != n) {
        gpc = pc;
        pc = fComp(x->fItem, n->fItem) ? kLeft_Child : kRight_Child;
        first = first && kLeft_Child == pc;
        last = last && kRight_Child == pc;
        gp = p;
        p = n;
        n = p->fChildren[pc];
    }
    if (last) {
        fLast = x;
    }
    if (first) {
        fFirst = x;
    }

    if (NULL == p) {
        fRoot = x;
        x->fColor = kBlack_Color;
        x->fParent = NULL;
        SkASSERT(1 == fCount);
        return Iter(returnNode, this);
    }
    p->fChildren[pc] = x;
    x->fColor = kRed_Color;
    x->fParent = p;

    do {
        // assumptions at loop start.
        SkASSERT(NULL != x);
        SkASSERT(kRed_Color == x->fColor);
        // can't have a grandparent but no parent.
        SkASSERT(!(NULL != gp && NULL == p));
        // make sure pc and gpc are correct
        SkASSERT(NULL == p  || p->fChildren[pc] == x);
        SkASSERT(NULL == gp || gp->fChildren[gpc] == p);

        // if x's parent is black then we didn't violate any of the
        // red/black properties when we added x as red.
        if (kBlack_Color == p->fColor) {
            return Iter(returnNode, this);
        }
        // gp must be valid because if p was the root then it is black
        SkASSERT(NULL != gp);
        // gp must be black since it's child, p, is red.
        SkASSERT(kBlack_Color == gp->fColor);


        // x and its parent are red, violating red-black property.
        Node* u = gp->fChildren[1-gpc];
        // if x's uncle (p's sibling) is also red then we can flip
        // p and u to black and make gp red. But then we have to recurse
        // up to gp since it's parent may also be red.
        if (NULL != u && kRed_Color == u->fColor) {
            p->fColor = kBlack_Color;
            u->fColor = kBlack_Color;
            gp->fColor = kRed_Color;
            x = gp;
            p = x->fParent;
            if (NULL == p) {
                // x (prev gp) is the root, color it black and be done.
                SkASSERT(fRoot == x);
                x->fColor = kBlack_Color;
                validate();
                return Iter(returnNode, this);
            }
            gp = p->fParent;
            pc = (p->fChildren[kLeft_Child] == x) ? kLeft_Child :
                                                    kRight_Child;
            if (NULL != gp) {
                gpc = (gp->fChildren[kLeft_Child] == p) ? kLeft_Child :
                                                          kRight_Child;
            }
            continue;
        } break;
    } while (true);
    // Here p is red but u is black and we still have to resolve the fact
    // that x and p are both red.
    SkASSERT(NULL == gp->fChildren[1-gpc] || kBlack_Color == gp->fChildren[1-gpc]->fColor);
    SkASSERT(kRed_Color == x->fColor);
    SkASSERT(kRed_Color == p->fColor);
    SkASSERT(kBlack_Color == gp->fColor);

    // make x be on the same side of p as p is of gp. If it isn't already
    // the case then rotate x up to p and swap their labels.
    if (pc != gpc) {
        if (kRight_Child == pc) {
            rotateLeft(p);
            Node* temp = p;
            p = x;
            x = temp;
            pc = kLeft_Child;
        } else {
            rotateRight(p);
            Node* temp = p;
            p = x;
            x = temp;
            pc = kRight_Child;
        }
    }
    // we now rotate gp down, pulling up p to be it's new parent.
    // gp's child, u, that is not affected we know to be black. gp's new
    // child is p's previous child (x's pre-rotation sibling) which must be
    // black since p is red.
    SkASSERT(NULL == p->fChildren[1-pc] ||
             kBlack_Color == p->fChildren[1-pc]->fColor);
    // Since gp's two children are black it can become red if p is made
    // black. This leaves the black-height of both of p's new subtrees
    // preserved and removes the red/red parent child relationship.
    p->fColor = kBlack_Color;
    gp->fColor = kRed_Color;
    if (kLeft_Child == pc) {
        rotateRight(gp);
    } else {
        rotateLeft(gp);
    }
    validate();
    return Iter(returnNode, this);
}


template <typename T, typename C>
void GrRedBlackTree<T,C>::rotateRight(Node* n) {
    /*            d?              d?
     *           /               /
     *          n               s
     *         / \     --->    / \
     *        s   a?          c?  n
     *       / \                 / \
     *      c?  b?              b?  a?
     */
    Node* d = n->fParent;
    Node* s = n->fChildren[kLeft_Child];
    SkASSERT(NULL != s);
    Node* b = s->fChildren[kRight_Child];

    if (NULL != d) {
        Child c = d->fChildren[kLeft_Child] == n ? kLeft_Child :
                                             kRight_Child;
        d->fChildren[c] = s;
    } else {
        SkASSERT(fRoot == n);
        fRoot = s;
    }
    s->fParent = d;
    s->fChildren[kRight_Child] = n;
    n->fParent = s;
    n->fChildren[kLeft_Child] = b;
    if (NULL != b) {
        b->fParent = n;
    }

    GR_DEBUGASSERT(validateChildRelations(d, true));
    GR_DEBUGASSERT(validateChildRelations(s, true));
    GR_DEBUGASSERT(validateChildRelations(n, false));
    GR_DEBUGASSERT(validateChildRelations(n->fChildren[kRight_Child], true));
    GR_DEBUGASSERT(validateChildRelations(b, true));
    GR_DEBUGASSERT(validateChildRelations(s->fChildren[kLeft_Child], true));
}

template <typename T, typename C>
void GrRedBlackTree<T,C>::rotateLeft(Node* n) {

    Node* d = n->fParent;
    Node* s = n->fChildren[kRight_Child];
    SkASSERT(NULL != s);
    Node* b = s->fChildren[kLeft_Child];

    if (NULL != d) {
        Child c = d->fChildren[kRight_Child] == n ? kRight_Child :
                                                   kLeft_Child;
        d->fChildren[c] = s;
    } else {
        SkASSERT(fRoot == n);
        fRoot = s;
    }
    s->fParent = d;
    s->fChildren[kLeft_Child] = n;
    n->fParent = s;
    n->fChildren[kRight_Child] = b;
    if (NULL != b) {
        b->fParent = n;
    }

    GR_DEBUGASSERT(validateChildRelations(d, true));
    GR_DEBUGASSERT(validateChildRelations(s, true));
    GR_DEBUGASSERT(validateChildRelations(n, true));
    GR_DEBUGASSERT(validateChildRelations(n->fChildren[kLeft_Child], true));
    GR_DEBUGASSERT(validateChildRelations(b, true));
    GR_DEBUGASSERT(validateChildRelations(s->fChildren[kRight_Child], true));
}

template <typename T, typename C>
typename GrRedBlackTree<T,C>::Node* GrRedBlackTree<T,C>::SuccessorNode(Node* x) {
    SkASSERT(NULL != x);
    if (NULL != x->fChildren[kRight_Child]) {
        x = x->fChildren[kRight_Child];
        while (NULL != x->fChildren[kLeft_Child]) {
            x = x->fChildren[kLeft_Child];
        }
        return x;
    }
    while (NULL != x->fParent && x == x->fParent->fChildren[kRight_Child]) {
        x = x->fParent;
    }
    return x->fParent;
}

template <typename T, typename C>
typename GrRedBlackTree<T,C>::Node* GrRedBlackTree<T,C>::PredecessorNode(Node* x) {
    SkASSERT(NULL != x);
    if (NULL != x->fChildren[kLeft_Child]) {
        x = x->fChildren[kLeft_Child];
        while (NULL != x->fChildren[kRight_Child]) {
            x = x->fChildren[kRight_Child];
        }
        return x;
    }
    while (NULL != x->fParent && x == x->fParent->fChildren[kLeft_Child]) {
        x = x->fParent;
    }
    return x->fParent;
}

template <typename T, typename C>
void GrRedBlackTree<T,C>::deleteAtNode(Node* x) {
    SkASSERT(NULL != x);
    validate();
    --fCount;

    bool hasLeft =  NULL != x->fChildren[kLeft_Child];
    bool hasRight = NULL != x->fChildren[kRight_Child];
    Child c = hasLeft ? kLeft_Child : kRight_Child;

    if (hasLeft && hasRight) {
        // first and last can't have two children.
        SkASSERT(fFirst != x);
        SkASSERT(fLast != x);
        // if x is an interior node then we find it's successor
        // and swap them.
        Node* s = x->fChildren[kRight_Child];
        while (NULL != s->fChildren[kLeft_Child]) {
            s = s->fChildren[kLeft_Child];
        }
        SkASSERT(NULL != s);
        // this might be expensive relative to swapping node ptrs around.
        // depends on T.
        x->fItem = s->fItem;
        x = s;
        c = kRight_Child;
    } else if (NULL == x->fParent) {
        // if x was the root we just replace it with its child and make
        // the new root (if the tree is not empty) black.
        SkASSERT(fRoot == x);
        fRoot = x->fChildren[c];
        if (NULL != fRoot) {
            fRoot->fParent = NULL;
            fRoot->fColor = kBlack_Color;
            if (x == fLast) {
                SkASSERT(c == kLeft_Child);
                fLast = fRoot;
            } else if (x == fFirst) {
                SkASSERT(c == kRight_Child);
                fFirst = fRoot;
            }
        } else {
            SkASSERT(fFirst == fLast && x == fFirst);
            fFirst = NULL;
            fLast = NULL;
            SkASSERT(0 == fCount);
        }
        delete x;
        validate();
        return;
    }

    Child pc;
    Node* p = x->fParent;
    pc = p->fChildren[kLeft_Child] == x ? kLeft_Child : kRight_Child;

    if (NULL == x->fChildren[c]) {
        if (fLast == x) {
            fLast = p;
            SkASSERT(p == PredecessorNode(x));
        } else if (fFirst == x) {
            fFirst = p;
            SkASSERT(p == SuccessorNode(x));
        }
        // x has two implicit black children.
        Color xcolor = x->fColor;
        p->fChildren[pc] = NULL;
        delete x;
        x = NULL;
        // when x is red it can be with an implicit black leaf without
        // violating any of the red-black tree properties.
        if (kRed_Color == xcolor) {
            validate();
            return;
        }
        // s is p's other child (x's sibling)
        Node* s = p->fChildren[1-pc];

        //s cannot be an implicit black node because the original
        // black-height at x was >= 2 and s's black-height must equal the
        // initial black height of x.
        SkASSERT(NULL != s);
        SkASSERT(p == s->fParent);

        // assigned in loop
        Node* sl;
        Node* sr;
        bool slRed;
        bool srRed;

        do {
            // When we start this loop x may already be deleted it is/was
            // p's child on its pc side. x's children are/were black. The
            // first time through the loop they are implict children.
            // On later passes we will be walking up the tree and they will
            // be real nodes.
            // The x side of p has a black-height that is one less than the
            // s side. It must be rebalanced.
            SkASSERT(NULL != s);
            SkASSERT(p == s->fParent);
            SkASSERT(NULL == x || x->fParent == p);

            //sl and sr are s's children, which may be implicit.
            sl = s->fChildren[kLeft_Child];
            sr = s->fChildren[kRight_Child];

            // if the s is red we will rotate s and p, swap their colors so
            // that x's new sibling is black
            if (kRed_Color == s->fColor) {
                // if s is red then it's parent must be black.
                SkASSERT(kBlack_Color == p->fColor);
                // s's children must also be black since s is red. They can't
                // be implicit since s is red and it's black-height is >= 2.
                SkASSERT(NULL != sl && kBlack_Color == sl->fColor);
                SkASSERT(NULL != sr && kBlack_Color == sr->fColor);
                p->fColor = kRed_Color;
                s->fColor = kBlack_Color;
                if (kLeft_Child == pc) {
                    rotateLeft(p);
                    s = sl;
                } else {
                    rotateRight(p);
                    s = sr;
                }
                sl = s->fChildren[kLeft_Child];
                sr = s->fChildren[kRight_Child];
            }
            // x and s are now both black.
            SkASSERT(kBlack_Color == s->fColor);
            SkASSERT(NULL == x || kBlack_Color == x->fColor);
            SkASSERT(p == s->fParent);
            SkASSERT(NULL == x || p == x->fParent);

            // when x is deleted its subtree will have reduced black-height.
            slRed = (NULL != sl && kRed_Color == sl->fColor);
            srRed = (NULL != sr && kRed_Color == sr->fColor);
            if (!slRed && !srRed) {
                // if s can be made red that will balance out x's removal
                // to make both subtrees of p have the same black-height.
                if (kBlack_Color == p->fColor) {
                    s->fColor = kRed_Color;
                    // now subtree at p has black-height of one less than
                    // p's parent's other child's subtree. We move x up to
                    // p and go through the loop again. At the top of loop
                    // we assumed x and x's children are black, which holds
                    // by above ifs.
                    // if p is the root there is no other subtree to balance
                    // against.
                    x = p;
                    p = x->fParent;
                    if (NULL == p) {
                        SkASSERT(fRoot == x);
                        validate();
                        return;
                    } else {
                        pc = p->fChildren[kLeft_Child] == x ? kLeft_Child :
                                                              kRight_Child;

                    }
                    s = p->fChildren[1-pc];
                    SkASSERT(NULL != s);
                    SkASSERT(p == s->fParent);
                    continue;
                } else if (kRed_Color == p->fColor) {
                    // we can make p black and s red. This balance out p's
                    // two subtrees and keep the same black-height as it was
                    // before the delete.
                    s->fColor = kRed_Color;
                    p->fColor = kBlack_Color;
                    validate();
                    return;
                }
            }
            break;
        } while (true);
        // if we made it here one or both of sl and sr is red.
        // s and x are black. We make sure that a red child is on
        // the same side of s as s is of p.
        SkASSERT(slRed || srRed);
        if (kLeft_Child == pc && !srRed) {
            s->fColor = kRed_Color;
            sl->fColor = kBlack_Color;
            rotateRight(s);
            sr = s;
            s = sl;
            //sl = s->fChildren[kLeft_Child]; don't need this
        } else if (kRight_Child == pc && !slRed) {
            s->fColor = kRed_Color;
            sr->fColor = kBlack_Color;
            rotateLeft(s);
            sl = s;
            s = sr;
            //sr = s->fChildren[kRight_Child]; don't need this
        }
        // now p is either red or black, x and s are red and s's 1-pc
        // child is red.
        // We rotate p towards x, pulling s up to replace p. We make
        // p be black and s takes p's old color.
        // Whether p was red or black, we've increased its pc subtree
        // rooted at x by 1 (balancing the imbalance at the start) and
        // we've also its subtree rooted at s's black-height by 1. This
        // can be balanced by making s's red child be black.
        s->fColor = p->fColor;
        p->fColor = kBlack_Color;
        if (kLeft_Child == pc) {
            SkASSERT(NULL != sr && kRed_Color == sr->fColor);
            sr->fColor = kBlack_Color;
            rotateLeft(p);
        } else {
            SkASSERT(NULL != sl && kRed_Color == sl->fColor);
            sl->fColor = kBlack_Color;
            rotateRight(p);
        }
    }
    else {
        // x has exactly one implicit black child. x cannot be red.
        // Proof by contradiction: Assume X is red. Let c0 be x's implicit
        // child and c1 be its non-implicit child. c1 must be black because
        // red nodes always have two black children. Then the two subtrees
        // of x rooted at c0 and c1 will have different black-heights.
        SkASSERT(kBlack_Color == x->fColor);
        // So we know x is black and has one implicit black child, c0. c1
        // must be red, otherwise the subtree at c1 will have a different
        // black-height than the subtree rooted at c0.
        SkASSERT(kRed_Color == x->fChildren[c]->fColor);
        // replace x with c1, making c1 black, preserves all red-black tree
        // props.
        Node* c1 = x->fChildren[c];
        if (x == fFirst) {
            SkASSERT(c == kRight_Child);
            fFirst = c1;
            while (NULL != fFirst->fChildren[kLeft_Child]) {
                fFirst = fFirst->fChildren[kLeft_Child];
            }
            SkASSERT(fFirst == SuccessorNode(x));
        } else if (x == fLast) {
            SkASSERT(c == kLeft_Child);
            fLast = c1;
            while (NULL != fLast->fChildren[kRight_Child]) {
                fLast = fLast->fChildren[kRight_Child];
            }
            SkASSERT(fLast == PredecessorNode(x));
        }
        c1->fParent = p;
        p->fChildren[pc] = c1;
        c1->fColor = kBlack_Color;
        delete x;
        validate();
    }
    validate();
}

template <typename T, typename C>
void GrRedBlackTree<T,C>::RecursiveDelete(Node* x) {
    if (NULL != x) {
        RecursiveDelete(x->fChildren[kLeft_Child]);
        RecursiveDelete(x->fChildren[kRight_Child]);
        delete x;
    }
}

#ifdef SK_DEBUG
template <typename T, typename C>
void GrRedBlackTree<T,C>::validate() const {
    if (fCount) {
        SkASSERT(NULL == fRoot->fParent);
        SkASSERT(NULL != fFirst);
        SkASSERT(NULL != fLast);

        SkASSERT(kBlack_Color == fRoot->fColor);
        if (1 == fCount) {
            SkASSERT(fFirst == fRoot);
            SkASSERT(fLast == fRoot);
            SkASSERT(0 == fRoot->fChildren[kLeft_Child]);
            SkASSERT(0 == fRoot->fChildren[kRight_Child]);
        }
    } else {
        SkASSERT(NULL == fRoot);
        SkASSERT(NULL == fFirst);
        SkASSERT(NULL == fLast);
    }
#if DEEP_VALIDATE
    int bh;
    int count = checkNode(fRoot, &bh);
    SkASSERT(count == fCount);
#endif
}

template <typename T, typename C>
int GrRedBlackTree<T,C>::checkNode(Node* n, int* bh) const {
    if (NULL != n) {
        SkASSERT(validateChildRelations(n, false));
        if (kBlack_Color == n->fColor) {
            *bh += 1;
        }
        SkASSERT(!fComp(n->fItem, fFirst->fItem));
        SkASSERT(!fComp(fLast->fItem, n->fItem));
        int leftBh = *bh;
        int rightBh = *bh;
        int cl = checkNode(n->fChildren[kLeft_Child], &leftBh);
        int cr = checkNode(n->fChildren[kRight_Child], &rightBh);
        SkASSERT(leftBh == rightBh);
        *bh = leftBh;
        return 1 + cl + cr;
    }
    return 0;
}

template <typename T, typename C>
bool GrRedBlackTree<T,C>::validateChildRelations(const Node* n,
                                                 bool allowRedRed) const {
    if (NULL != n) {
        if (NULL != n->fChildren[kLeft_Child] ||
            NULL != n->fChildren[kRight_Child]) {
            if (n->fChildren[kLeft_Child] == n->fChildren[kRight_Child]) {
                return validateChildRelationsFailed();
            }
            if (n->fChildren[kLeft_Child] == n->fParent &&
                NULL != n->fParent) {
                return validateChildRelationsFailed();
            }
            if (n->fChildren[kRight_Child] == n->fParent &&
                NULL != n->fParent) {
                return validateChildRelationsFailed();
            }
            if (NULL != n->fChildren[kLeft_Child]) {
                if (!allowRedRed &&
                    kRed_Color == n->fChildren[kLeft_Child]->fColor &&
                    kRed_Color == n->fColor) {
                    return validateChildRelationsFailed();
                }
                if (n->fChildren[kLeft_Child]->fParent != n) {
                    return validateChildRelationsFailed();
                }
                if (!(fComp(n->fChildren[kLeft_Child]->fItem, n->fItem) ||
                      (!fComp(n->fChildren[kLeft_Child]->fItem, n->fItem) &&
                       !fComp(n->fItem, n->fChildren[kLeft_Child]->fItem)))) {
                    return validateChildRelationsFailed();
                }
            }
            if (NULL != n->fChildren[kRight_Child]) {
                if (!allowRedRed &&
                    kRed_Color == n->fChildren[kRight_Child]->fColor &&
                    kRed_Color == n->fColor) {
                    return validateChildRelationsFailed();
                }
                if (n->fChildren[kRight_Child]->fParent != n) {
                    return validateChildRelationsFailed();
                }
                if (!(fComp(n->fItem, n->fChildren[kRight_Child]->fItem) ||
                      (!fComp(n->fChildren[kRight_Child]->fItem, n->fItem) &&
                       !fComp(n->fItem, n->fChildren[kRight_Child]->fItem)))) {
                    return validateChildRelationsFailed();
                }
            }
        }
    }
    return true;
}
#endif

#endif
