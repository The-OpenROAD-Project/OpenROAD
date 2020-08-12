#ifndef __SKYLINE_CONTOUR__
#define __SKYLINE_CONTOUR__ 0

#include <iostream>
#include "ABKCommon/abkcommon.h"
#include "btree.h"

// A structure for building 
struct Block { 
  float left;  // x coordinate of left side 
  float ht;    // height 
  float right; // x coordinate of right side 
}; 

// A strip in skyline 
class Strip { 
  private:
  float left;  // x coordinate of left side 
  float ht; // height
  int index; // original index

  public: 
  Strip(float _left = 0.0f, float _height = 0.0f, int _index = -1);
 
  void print(); 
  friend class Skyline;
  friend class SkylineContour; 
}; 

// Skyline:  To represent Output (An array of strips) 
class Skyline { 
  private:
  Strip *arr;   // Array of strips 
  int capacity; // Capacity of strip array 
  int n;   // Actual number of strips in array

  public:
  // Constructor 
  Skyline();
  Skyline( const Skyline & prev ); 
  Skyline(int cap); 
  ~Skyline();

  int count()  { return n;   } 

  // A function to merge another skyline 
  // to this skyline 
  Skyline* Merge(Skyline *other); 

  // Function to add a strip 'st' to array 
  void append(Strip *st);

  // A utility function to print all strips of 
  // skyline 
  void print();

  Skyline* copy();

  friend class BTree; 
  friend class SkylineContour; 
};

class SkylineContour {
  public:
  // Incremental BTree Node Information
  class BTreeNode {
    public: 
      float left;
      float right;
      float height;
      int idx;
      BTreeNode(float _left, float _right, float _height, int _idx) :
        left(_left), right(_right), height(_height), idx(_idx) {};
  };

  // Contour Node Information
  class ContourNode { 
    public: 
      float begin;
      float end;
      float height;
  };
  private: 
//    Strip strips_[5000];
    Skyline* skyline_;
    float width_;
    float height_;

    // contourInfo_ is updated from skyline_
    std::vector<BTreeNode> bTreeInfo_; 

  public:
    SkylineContour();
    SkylineContour(int capacity);
    SkylineContour(const SkylineContour& prev);
    bool operator =(const SkylineContour& prev);
    SkylineContour(const BTree& bTree, bool isRotate = false); 
    ~SkylineContour(); 

    void Clear();

    void InsertBtreeNode( float _left, float _right, 
        float _height, int _idx );
    void EvaluateContour();
    float Width() { return width_; };
    float Height() { return height_; };
    float GetHeight(float left, float right);
    float GetContourArea(); 
    std::pair<float, int> GetHeightWithIdx(float left, float right);
  
};
#endif
