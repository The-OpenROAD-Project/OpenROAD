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

/*
   1998.5.21  Paul Tucker

   Red-Black binary tree methods in support of single-row
   net optimization.  Basic algorithms copied from
   Cormen, Leiserson & Rivest, with polarity and count field
   maintenance added.
*/

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "rbtree.h"

using std::ostream;
using std::endl;

void RBTreeNode::simpleInsert(RBTreeNode *n) {
        // abkassert(n->minusCount + n->plusCount == 1, "duh?");
        minusCount += n->minusCount;
        plusCount += n->plusCount;

        if (n->key <= key) {
                if (left == NULL) {
                        left = n;
                        n->parent = this;
                } else
                        left->simpleInsert(n);
        } else {
                if (right == NULL) {
                        right = n;
                        n->parent = this;
                } else
                        right->simpleInsert(n);
        }
}

ostream &operator<<(ostream &out, RBTreeNode *node) {
        if (node->left != NULL) out << node->left;
        out << "   <k " << node->key << ", p " << node->polarity << ", m " << node->minusCount << ", p " << node->plusCount << ", addr:" << (int *)node << ", left " << (int *)node->left << ", right " << (int *)node->right << " " << (node->color == RBTreeNode::BLACK ? "BL" : "RD") << "> " << endl;
        if (node->right != NULL) out << node->right;
        return out;
}

RBTreeNode *RBTreeNode::leftRotate()
    /* left rotation raises the right child to parent, and
       makes this node the left child.
             T                R
           /   \            /   \
          A     R  ==>     T     C
               / \        / \
              B   C      A   B
    */
{
        abkassert(right != NULL, "Left rotation applied to invalid subtree");
        RBTreeNode *R = right;
        // cout << "leftRotate " << (unsigned*)this << endl;

        // this becomes parent of R's left child
        right = R->left;
        if (right != NULL) {
                right->parent = this;
                minusCount = right->minusCount;
                plusCount = right->plusCount;
        } else {
                minusCount = plusCount = 0;
        }
        if (left != NULL) {
                minusCount += left->minusCount;
                plusCount += left->plusCount;
        }
        if (polarity == -1)
                ++minusCount;
        else
                ++plusCount;

        // R becomes child of this's parent
        if (parent != NULL) {
                if (parent->left == this)
                        parent->left = R;
                else
                        parent->right = R;
        }
        R->parent = parent;

        // this becomes left child of R
        R->left = this;
        parent = R;
        R->minusCount = minusCount;
        R->plusCount = plusCount;
        if (R->right != NULL) {
                R->minusCount += R->right->minusCount;
                R->plusCount += R->right->plusCount;
        }
        if (R->polarity == -1)
                R->minusCount += 1;
        else
                R->plusCount += 1;

        return R;
}

RBTreeNode *RBTreeNode::rightRotate()
    /*       T              R
           /   \          /   \
          R     C  ==>   A     T
         / \                  / \
        A   B                B   C
    */
{
        abkassert(left != NULL, "Right rotation applied to invalid subtree");
        RBTreeNode *R = left;

        // cout << "Right rotate " << (unsigned*)this << "; result: " << endl;

        // this becomes parent of R's right child
        left = R->right;
        if (left != NULL) {
                left->parent = this;
                minusCount = left->minusCount;
                plusCount = left->plusCount;
        } else {
                minusCount = plusCount = 0;
        }
        if (right != NULL) {
                minusCount += right->minusCount;
                plusCount += right->plusCount;
        }
        if (polarity == -1)
                ++minusCount;
        else
                ++plusCount;

        // R becomes child of this's parent
        if (parent != NULL) {
                if (parent->right == this)
                        parent->right = R;
                else
                        parent->left = R;
        }
        R->parent = parent;

        // this becomes right child of R
        R->right = this;
        parent = R;
        R->minusCount = minusCount;
        R->plusCount = plusCount;
        if (R->left != NULL) {
                R->minusCount += R->left->minusCount;
                R->plusCount += R->left->plusCount;
        }
        if (R->polarity == -1)
                R->minusCount += 1;
        else
                R->plusCount += 1;

        return R;
}

RBTreeNode *RBTreeNode::insert(RBTreeNode *n)
    // This method is only invoked on the root of the tree.
{
        RBTreeNode *root = this;
        /*  this->check(NULL);
            cout << "Inserting " << n;
            cout << "into " << this;
        */

        // First do ordinary binary tree insertion
        simpleInsert(n);

        // Then restore the red-black property.
        RBTreeNode *x = n;
        n->color = RED;

        while ((x != this) && (x->parent->color == RED)) {
                abkassert(x->parent->parent != NULL, "Red-Black violation");
                if (x->parent == x->parent->parent->left) {
                        RBTreeNode *y = x->parent->parent->right;
                        if ((y != NULL) && (y->color == RED)) {
                                x->parent->color = BLACK;
                                y->color = BLACK;
                                x->parent->parent->color = RED;
                                x = x->parent->parent;
                        } else {
                                if (x == x->parent->right) {
                                        x = x->parent;
                                        if (x == root)
                                                root = x->leftRotate();
                                        else
                                                x->leftRotate();
                                }
                                x->parent->color = BLACK;
                                x->parent->parent->color = RED;
                                if (x->parent->parent == root)
                                        root = x->parent->parent->rightRotate();
                                else
                                        x->parent->parent->rightRotate();
                        }
                } else {
                        RBTreeNode *y = x->parent->parent->left;
                        if ((y != NULL) && (y->color == RED)) {
                                x->parent->color = BLACK;
                                y->color = BLACK;
                                x->parent->parent->color = RED;
                                x = x->parent->parent;
                        } else {
                                if (x == x->parent->left) {
                                        x = x->parent;
                                        if (x == root)
                                                root = x->rightRotate();
                                        else
                                                x->rightRotate();
                                }
                                x->parent->color = BLACK;
                                x->parent->parent->color = RED;
                                if (x->parent->parent == root)
                                        root = x->parent->parent->leftRotate();
                                else
                                        x->parent->parent->leftRotate();
                        }
                }
        }
        root->color = BLACK;
        // When debugged, get rid of next line
        // root->check(NULL);
        return root;
}

unsigned RBTreeNode::check(RBTreeNode *mom)
    // Complains about various violations of red-black properties
    // and tree well-formedness
{
        abkfatal(false, "this shouldn't execute");
        unsigned leftCount, rightCount;

        abkassert(parent == mom, "Bad parent pointer");
        (void)mom;  // unused in optimized version

        if (color == RED) {
                if (((left != NULL) && (left->color == RED)) || ((right != NULL) && (right->color == RED))) abkassert(false, "Violation! Red node has a red child.");
        }

        unsigned pcount = 0;
        unsigned mcount = 0;
        if (left != NULL) {
                pcount += left->plusCount;
                mcount += left->minusCount;
        }
        if (right != NULL) {
                pcount += right->plusCount;
                mcount += right->minusCount;
        }
        if (polarity == -1) mcount++;
        if (polarity == 1) pcount++;

        abkassert(mcount == minusCount, "Minuscount wrong");
        abkassert(pcount == plusCount, "Pluscount wrong");

        if (left != NULL)
                leftCount = left->check(this);
        else
                leftCount = 1;  // NULL counts as black
        if (right != NULL)
                rightCount = right->check(this);
        else
                rightCount = 1;
        if (rightCount != leftCount) abkassert(false, "Violation! black heights differ.");

        return leftCount + (color == BLACK ? 1 : 0);
}

RBInterval RBTreeNode::findMin(const RBInterval &bounds, unsigned succMinus, unsigned precPlus)
    // Find the min-cost interval.
    // This is the interval with 0 slope.
    // If no interval with positive length exists, we
    // return the last point of negative slope, or first
    // point of positive slope.
{
        unsigned rightMinus = 0;
        unsigned leftPlus = 0;

        if (right != NULL) rightMinus = right->minusCount;
        if (left != NULL) leftPlus = left->plusCount;

        int slope = (leftPlus + precPlus) - (rightMinus + succMinus) + (polarity < 0 ? 0 : 1);

        if (slope == 0) {
                RBTreeNode *next = successor();
                if (next != NULL)
                        return RBInterval(key, next->key);
                else
                        return RBInterval(key, bounds.high);
        } else if (slope < 0)
                if (right != NULL)
                        return right->findMin(bounds, succMinus, precPlus + leftPlus + (polarity < 0 ? 0 : 1));
                else
                        return RBInterval(key, bounds.high);
        else if (left != NULL)
                return left->findMin(bounds, succMinus + rightMinus + (polarity < 0 ? 1 : 0), precPlus);
        else
                return RBInterval(bounds.low, key);
}

RBTreeNode *RBTreeNode::successor()
    // The successor of a node is either the leftmost node
    // in its right subtree, or its first ancestor on
    // a left-child link.
{
        if (right != NULL) {
                RBTreeNode *x = right;
                while (x->left != NULL) x = x->left;
                return x;
        } else if (parent != NULL) {
                if (this == parent->left) return parent;

                RBTreeNode *x = parent;
                while (x->parent != NULL)
                        if (x == x->parent->left)
                                return x->parent;
                        else
                                x = x->parent;
        }
        return NULL;
}

RBTreeNode *RBTreeNode::predecessor()
    // The predecessor is either the rightmost node in the left subtree,
    // or the first ancestor on a right-child link.
{
        if (left != NULL) {
                RBTreeNode *x = left;
                while (x->right != NULL) x = x->right;
                return x;
        } else if (parent != NULL) {
                if (this == parent->right) return parent;

                RBTreeNode *x = parent;
                while (x->parent != NULL)
                        if (x == x->parent->right)
                                return x->parent;
                        else
                                x = x->parent;
        }
        return NULL;
}

void RBTreeNode::applyPostOrder(RBTFunction &f) {
        if (left != NULL) left->applyPostOrder(f);
        if (right != NULL) right->applyPostOrder(f);

        f(this);
}

RBTreeNode *RBTreeNode::find(int p, double k)
    // Find the first node with polarity p and key k.
{
        RBTreeNode *result = NULL;
        if (key == k) {
                // We're in the right neighborhood, but multiple nodes with
                // this key can exist.
                if (polarity == p)
                        result = this;
                else if (left != NULL)
                        result = left->find(p, k);
                if (result == NULL) {
                        if (right != NULL) result = right->find(p, k);
                }
        } else {
                if (key < k)
                        if (right != NULL)
                                result = right->find(p, k);
                        else
                                result = NULL;
                else if (left != NULL)
                        result = left->find(p, k);
                else
                        result = NULL;
        }
        return result;
}

RBTreeNode *RBTreeNode::findAndDelete(int p, double k)
    // Find the first node with polarity p and key k, and
    // delete it.  Return the new root.  Node must exist.
{
        RBTreeNode *root = this;

        // this->check(NULL);

        RBTreeNode *z = find(p, k);
        abkfatal(z != NULL, "Request to delete a node not found");
        RBTreeNode *y;  // The node that will take z's place
        RBTreeNode *x;  // the child that will take y's place

        if ((z->left == NULL) || (z->right == NULL))
                y = z;
        else
                y = z->successor();  // must be a descendent of z
        if (y->left != NULL)
                x = y->left;
        else
                x = y->right;

        // cout << "delete x: " << (unsigned*)x << " y: " << (unsigned*)y
        //  << " z: " << (unsigned*)z << endl;

        if (x != NULL) x->parent = y->parent;
        if (y->parent == NULL)
                root = x;
        else {
                if (y == y->parent->left)
                        y->parent->left = x;
                else
                        y->parent->right = x;
        }

        if (y != z) {
                z->key = y->key;
                z->polarity = y->polarity;
                if (x != NULL)
                        x->fixupPlusMinus();
                else if (y->parent != NULL)
                        y->parent->fixupPlusMinus();
        }

        z->fixupPlusMinus();

        if (y->color == BLACK) deleteFixup(root, x, y->parent);

        y->left = y->right = NULL;
        delete y;

        // Get rid of this when finished debugging!!
        // cout << "result tree: " << endl;
        // if (root == NULL)
        //  cout << "NULL" << endl;
        // else
        //  cout << root;
        // if (root != NULL)
        //  root->check(NULL);

        return root;
}

void RBTreeNode::fixupPlusMinus() {
        plusCount = (polarity == 1 ? 1 : 0) + (left == NULL ? 0 : left->plusCount) + (right == NULL ? 0 : right->plusCount);
        minusCount = (polarity == -1 ? 1 : 0) + (left == NULL ? 0 : left->minusCount) + (right == NULL ? 0 : right->minusCount);
        if (parent != NULL) parent->fixupPlusMinus();
}

void RBTreeNode::deleteFixup(RBTreeNode *&root, RBTreeNode *x, RBTreeNode *y)
    // The CLR text explains RB tree operations using a sentinel
    // for NIL.  I use NULL, so a few operations are slighly more
    // complex.  Basically, we just have to keep track of x and
    // its parent separately, in case x is NULL.
    // Also, note that the color of NULL is always BLACK.
    // x is the child of the spliced out node, and
    // y is its (new) parent.  x may be NULL, if the spliced out
    // node had no children.  If y is null, then the spliced out
    // node was the root, and x is the new root.
{
        // cout << "deletefixup: " << (unsigned*)x << "  " << (unsigned*)y <<
        // endl;

        while ((y != NULL) && ((x == NULL) || (x->color == BLACK))) {
                if (x == y->left) {
                        RBTreeNode *w = y->right;
                        if ((w != NULL) && (w->color == RED)) {
                                w->color = BLACK;
                                y->color = RED;
                                if (y == root)
                                        root = y->leftRotate();
                                else
                                        y->leftRotate();
                                w = y->right;
                        }
                        if (((w->left == NULL) || (w->left->color == BLACK)) && ((w->right == NULL) || (w->right->color == BLACK))) {
                                w->color = RED;
                                x = y;
                                y = y->parent;
                        } else {
                                if ((w->right == NULL) || (w->right->color == BLACK)) {
                                        w->left->color = BLACK;
                                        w->color = RED;
                                        if (w == root)
                                                root = w->rightRotate();
                                        else
                                                w->rightRotate();
                                        w = y->right;
                                }
                                w->color = y->color;
                                y->color = BLACK;
                                if (w->right != NULL) w->right->color = BLACK;
                                if (y == root)
                                        root = y->leftRotate();
                                else
                                        y->leftRotate();
                                x = root;
                                y = NULL;
                        }
                } else {
                        RBTreeNode *w = y->left;
                        if ((w != NULL) && (w->color == RED)) {
                                w->color = BLACK;
                                y->color = RED;
                                if (y == root)
                                        root = y->rightRotate();
                                else
                                        y->rightRotate();
                                w = y->left;
                        }
                        if (((w->right == NULL) || (w->right->color == BLACK)) && ((w->left == NULL) || (w->left->color == BLACK))) {
                                w->color = RED;
                                x = y;
                                y = y->parent;
                        } else {
                                if ((w->left == NULL) || (w->left->color == BLACK)) {
                                        w->right->color = BLACK;
                                        w->color = RED;
                                        if (w == root)
                                                root = w->leftRotate();
                                        else
                                                w->leftRotate();
                                        w = y->left;
                                }
                                w->color = y->color;
                                y->color = BLACK;
                                if (w->left != NULL) w->left->color = BLACK;
                                if (y == root)
                                        root = y->rightRotate();
                                else
                                        y->rightRotate();
                                x = root;
                                y = NULL;
                        }
                }
        }  // while

        if (x != NULL) x->color = BLACK;
}

double RBTreeNode::findCost(double x)
    // This is not an ordinary tree operation, but a special
    // operation in support of cost curve evaluation.
    // The param x is the point at which we want to evaluate
    // the cost curve.  First we search down in the tree
    // to find the interval (and its slope) in which x lies,
    // then we scan left or right in order to find the 0-slope
    // (min cost) interval, and total the sum cost to x.
{
        RBTreeNode *n = this;

        // Find a bounding point of the interval containing x,
        // and get the slope.
        bool found = false;
        int minusRight = 0;
        int plusLeft = 0;
        while (!found) {
                if (n->key < x) {
                        if (n->polarity == 1) plusLeft += 1;
                        if (n->left != NULL) plusLeft += n->left->plusCount;
                        if (n->right != NULL)
                                n = n->right;
                        else
                                found = true;
                } else if (n->key > x) {
                        if (n->polarity == -1) minusRight += 1;
                        if (n->right != NULL) minusRight += n->right->minusCount;
                        if (n->left != NULL)
                                n = n->left;
                        else
                                found = true;
                } else  // (n->key == x)
                {
                        found = true;
                        if (n->right != NULL) minusRight += n->right->minusCount;
                        if (n->left != NULL) plusLeft += n->left->plusCount;
                        if ((plusLeft - minusRight) < 0) {
                                if (n->polarity == 1) plusLeft += 1;
                        } else if ((plusLeft - minusRight) > 0) {
                                if (n->polarity == -1) minusRight += 1;
                        }
                }
        }
        // Calculate placement cost of x, relative to slope-0 interval.
        int slope = plusLeft - minusRight;
        double lastPoint = x;
        double cost = 0;

        if (slope < 0) {
                if (n->key > x) {
                        cost += slope * (x - n->key);
                        lastPoint = n->key;
                        slope += 1;
                }
                while (slope < 0) {
                        n = n->successor();
                        cost += slope * (lastPoint - n->key);
                        slope += 1;
                        lastPoint = n->key;
                }
        } else if (slope > 0) {
                if (n->key < x) {
                        cost += slope * (x - n->key);
                        lastPoint = n->key;
                        slope -= 1;
                }
                while (slope > 0) {
                        n = n->predecessor();
                        cost += slope * (lastPoint - n->key);
                        slope -= 1;
                        lastPoint = n->key;
                }
        }

        return cost;
}

int RBTreeNode::findSlope(double x)
    // Similar to findCost, but just returns the slope.
{
        RBTreeNode *n = this;

        // Find a bounding point of the interval containing x,
        // and get the slope.
        bool found = false;
        unsigned minusRight = 0;
        unsigned plusLeft = 0;
        while (!found) {
                if (n->key < x) {
                        if (n->polarity == 1) plusLeft += 1;
                        if (n->left != NULL) plusLeft += n->left->plusCount;
                        if (n->right != NULL)
                                n = n->right;
                        else
                                found = true;
                } else if (n->key > x) {
                        if (n->polarity == -1) minusRight += 1;
                        if (n->right != NULL) minusRight += n->right->minusCount;
                        if (n->left != NULL)
                                n = n->left;
                        else
                                found = true;
                } else
                        found = true;
        }

        int slope = plusLeft - minusRight;

        // There's a little fixup left, because we may be off by 1.

        if (slope <= 0) {
                if (n->key > x) slope += 1;
        } else if (n->key < x)
                slope -= 1;

        return slope;
}
