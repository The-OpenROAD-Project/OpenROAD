/**************************************************************************
***
*** Copyright (c) 1995-2000 Regents of the University of California,
***               Andrew E. Caldwell, Andrew B. Kahng and Igor L. Markov
*** Copyright (c) 2000-2007 Regents of the University of Michigan,
***               Saurabh N. Adya, Jarrod A. Roy, David A. Papa and
***               Igor L. Markov
***
***  Contact author(s): abk@cs.ucsd.edu, imarkov@umich.edu
***  Original Affiliation:   UCLA, Computer Science Department,
***                          Los Angeles, CA 90095-1596 USA
***
***  Permission is hereby granted, free of charge, to any person obtaining
***  a copy of this software and associated documentation files (the
***  "Software"), to deal in the Software without restriction, including
***  without limitation
***  the rights to use, copy, modify, merge, publish, distribute, sublicense,
***  and/or sell copies of the Software, and to permit persons to whom the
***  Software is furnished to do so, subject to the following conditions:
***
***  The above copyright notice and this permission notice shall be included
***  in all copies or substantial portions of the Software.
***
*** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
*** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
*** OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
*** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
*** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
*** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
*** THE USE OR OTHER DEALINGS IN THE SOFTWARE.
***
***
***************************************************************************/

//! author="Paul Tucker 1998.5.21"
//! CONTACTS=" Igor ABK"

/*
   1998.5.21  Paul Tucker

   Red-Black tree datatype for single-row net optimization.

   Standard methods have been adapted for this application.
*/

#ifndef __RBTREE_H__
#define __RBTREE_H__

#include "ABKCommon/abkcommon.h"
#include <iostream>

/*#include "interval.h"*/

/*------------------ class declarations -----------------------*/
class RBInterval;
class RBTreeNode;
class RBTFunction;
class AbsorbFunction;

/*------------------- class definitions -------------------------*/

//: Interval of Red-Black tree
class RBInterval {
       public:
        double low;
        double high;
        RBInterval(double l, double h) : low(l), high(h) {}
};

//: Node in red-black tree
class RBTreeNode {
        friend class AbsorbFunction;

       protected:
        enum Color {
                BLACK,
                RED
        };

        RBTreeNode* left;
        RBTreeNode* right;
        RBTreeNode* parent;
        Color color;
        double key;           // an x coordinate
        int polarity;         // either -1 or +1
        unsigned plusCount;   // positive polarity in this subtree
        unsigned minusCount;  // negative polarity in this subtree

        void simpleInsert(RBTreeNode* n);
        void fixupPlusMinus();
        void deleteFixup(RBTreeNode*& root, RBTreeNode* x, RBTreeNode* y);
        std::ostream& inorderPrint(std::ostream& out);

        friend std::ostream& operator<<(std::ostream& out, RBTreeNode* node);

       public:
        RBTreeNode(double k, int p)
            : left(NULL),
              right(NULL),
              parent(NULL),
              color(BLACK),
              key(k),
              polarity(p),
              plusCount(0),
              minusCount(0)
              // initial a node of RB-tree by default value
        {
                /*abkassert((p == 1) || (p == -1), "Bad polarity arg");*/
                if (p == -1)
                        minusCount = 1;
                else
                        plusCount = 1;
        }

        ~RBTreeNode() {
                if (left != NULL) delete left;
                if (right != NULL) delete right;
        }

        RBTreeNode(const RBTreeNode& n) : left(n.left == NULL ? NULL : new RBTreeNode(*n.left)), right(n.right == NULL ? NULL : new RBTreeNode(*n.right)), parent(NULL), color(n.color), key(n.key), polarity(n.polarity), plusCount(n.plusCount), minusCount(n.minusCount) {
                if (left != NULL) left->parent = this;
                if (right != NULL) right->parent = this;
        }

        unsigned check(RBTreeNode*);
        RBTreeNode* leftRotate();
        RBTreeNode* rightRotate();
        RBTreeNode* insert(RBTreeNode* n);
        RBTreeNode* find(int p, double k);
        RBTreeNode* successor();
        RBTreeNode* predecessor();
        double findCost(double x);
        int findSlope(double x);
        void applyPostOrder(RBTFunction& f);
        RBInterval findMin(const RBInterval& bounds, unsigned succMinus = 0, unsigned precPlus = 0);
        RBTreeNode* findAndDelete(int p, double k);
};

//: Base class for RB-tree operations
class RBTFunction {
       public:
        virtual void operator()(RBTreeNode* t) = 0;
        virtual ~RBTFunction() {}
};

//: This function object is used to absorb the nodes from one
// tree into another, when using applyPostOrder to iterate
// over the tree being absorbed.
class AbsorbFunction : public RBTFunction {
       protected:
        RBTreeNode* root;
        double delta;
        RBInterval bounds;

       public:
        AbsorbFunction(RBTreeNode* r, double del, const RBInterval& b) : root(r), delta(del), bounds(b) {}

        void operator()(RBTreeNode* other) {
                if (other->polarity == -1) {
                        other->minusCount = 1;
                        other->plusCount = 0;
                } else {
                        other->minusCount = 0;
                        other->plusCount = 1;
                }
                other->right = other->left = other->parent = NULL;

                other->key -= delta;
                // I thought about trimming points to new bounds here,
                // but points already in the left tree may be out of
                // bounds...it's better to have the merged in points
                // in their original frame of reference, and fix up
                // the bounds when the minimum interval is queried.
                // if (other->key > bounds.high)
                //  other->key = bounds.high;
                // if (other->key < bounds.low)
                //  other->key = bounds.low;

                if (root != NULL)
                        root = root->insert(other);
                else {
                        root = other;
                        other->color = other->BLACK;
                        other->parent = NULL;
                }
        }

        RBTreeNode* getRoot() { return root; }
};

#endif
