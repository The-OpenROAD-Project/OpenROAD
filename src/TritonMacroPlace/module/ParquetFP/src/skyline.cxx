#include <iostream>
#include "skyline.h"
#include "ABKCommon/abkcommon.h"

using std::cout;
using std::endl;
// A divide and conquer based C++ program to find skyline of given 
// buildings 

//
// Strip Structure Definition
//
Strip::Strip(float _left, float _height, int _index) {
  left = _left; 
  ht = _height; 
  index = _index;
}
void Strip::print() {
  cout << "Left: " << left << " Height: " << ht << " index: " << index << endl;
}

//
// Skyline Structure Definition
//

Skyline::Skyline(const Skyline &prev) {
  this->capacity = prev.n;
  this->n = prev.n;

  this->arr = new Strip[this->capacity];
  for(int i=0; i< this->n; i++) {
    this->arr[i] = prev.arr[i]; 
  }
}

Skyline::Skyline() : arr(0), capacity(0), n(0) {}

Skyline::Skyline(int cap) {
  capacity = cap; 
//  cout << "CAP: " << cap << endl;
  arr = new Strip[cap]; 
  n = 0; 
}

Skyline::~Skyline() {
  if( arr ) {
    delete[] arr;
  }
  arr = 0;
  capacity = 0;
}

void Skyline::append(Strip* st) {
  // Check for redundant strip, a strip is 
  // redundant if it has same height or left as previous 
  if (n > 0 && arr[n-1].ht == st->ht) {
    return; 
  }
  if (n > 0 && arr[n-1].left == st->left) {
    //      arr[n-1].ht = max(arr[n-1].ht, st->ht); 
    if( arr[n-1].ht < st->ht ) {
      arr[n-1].ht = st->ht;
      arr[n-1].index = st->index;
    }
    return; 
  } 
  arr[n] = *st; 
  n++; 
}

void Skyline::print() {
  for (int i=0; i<n; i++) { 
    cout << i << " Left:" << arr[i].left << " Height:"
      << arr[i].ht << " Index:" << arr[i].index << endl; 
  } 
  cout << endl;
}

// This function returns skyline for a given array of buildings 
// arr[l..h].  This function is similar to mergeSort(). 
Skyline *findSkyline(
    std::vector<SkylineContour::BTreeNode>& arr, 
    int l, int h) {

  if (l == h) { 
    Skyline *res = new Skyline(2); 
    Strip left(arr[l].left, arr[l].height, arr[l].idx);
    Strip right(arr[l].right, 0, arr[l].idx);

    res->append(&left); 
    res->append(&right);
//    cout << "recursive End: " << endl;
//    res->print(); 
    return res; 
  } 

  int mid = (l + h)/2; 

  // Recur for left and right halves and merge the two results 
  Skyline *sl = findSkyline(arr, l, mid); 
  Skyline *sr = findSkyline(arr, mid+1, h); 
  Skyline *res = sl->Merge(sr); 

  // To avoid memory leak 
  delete sl; 
  delete sr; 

  // Return merged skyline 
  return res; 
} 

// Similar to merge() in MergeSort 
// This function merges another skyline 'other' to the skyline 
// for which it is called.  The function returns pointer to 
// the resultant skyline 
Skyline *Skyline::Merge(Skyline *other) { 
//  cout << "Merge: below Two Lists" << endl;
//  cout << "this" << endl;
//  this->print(); 
//  cout << "other" << endl;
//  other->print();
//  cout << endl;
  
  // Create a resultant skyline with capacity as sum of two 
  // skylines 
  Skyline *res = new Skyline(this->n + other->n); 

  

  // To store current heights of two skylines 
  float h1 = 0, h2 = 0; 

  // Indexes of strips in two skylines 
  int i = 0, j = 0;
  int idx1 = 0, idx2 = 0;

  while (i < this->n && j < other->n) { 
    // Compare x coordinates of left sides of two 
    // skylines and put the smaller one in result 
    if (this->arr[i].left < other->arr[j].left) { 
      float x1 = this->arr[i].left; 
      h1 = this->arr[i].ht; 
      idx1 = this->arr[i].index;

      // Choose height as max of two heights 
      float maxh = fmax(h1, h2);
      
//      cout << "icase" << endl;
//      cout << "i-j: " << i << " " << j << endl;
//      cout << "h1-h2: " << h1 << " " << h2 << " " << endl;
//      cout << "idx1-idx2: " << idx1 << " " << idx2 << endl << endl; 
      
      Strip curStrip(x1, maxh, (h1 > h2)? idx1 : idx2);
      res->append(&curStrip);
      i++; 
    } 
    else if (this->arr[i].left > other->arr[j].left )
    { 
      float x2 = other->arr[j].left; 
      h2 = other->arr[j].ht;
      idx2 = other->arr[j].index;

      float maxh = fmax(h1, h2); 
      
//      cout << "jcase" << endl;
//      cout << "i-j: " << i << " " << j << endl;
//      cout << "h1-h2: " << h1 << " " << h2 << endl; 
//      cout << "idx1-idx2: " << idx1 << " " << idx2 << endl << endl; 
      
      Strip curStrip(x2, maxh, (h1 > h2)? idx1 : idx2);
      res->append(&curStrip);
      j++; 
    }
    // below x1 and x2 is same cases
    else {
      float x1 = this->arr[i].left;
      h1 = this->arr[i].ht; 
      idx1 = this->arr[i].index;
      
//      float x2 = other->arr[j].left; 
      h2 = other->arr[j].ht;
      idx2 = other->arr[j].index;
      
      float maxh = fmax(h1, h2);

      // here x1 and x2 is same
      Strip curStrip(x1, maxh, (h1 > h2)? idx1 : idx2);
      res->append(&curStrip);
      i++;
      j++;
    } 
  } 

  // If there are strips left in this skyline or other 
  // skyline 
  while (i < this->n) 
  { 
    res->append(&arr[i]); 
    i++; 
  } 
  while (j < other->n) 
  { 
    res->append(&other->arr[j]); 
    j++; 
  }
  
//  cout << "after merged: " << endl;
//  res->print(); 
  return res; 
} 

Skyline* Skyline::copy() {
  Skyline* newObj = new Skyline(n);
  for(int i=0; i<n; i++) {
    newObj->arr[i] = this->arr[i]; 
  }
  return newObj;
}

SkylineContour::SkylineContour() 
  : skyline_(0), width_(FLT_MIN), height_(FLT_MIN) {}

SkylineContour::SkylineContour(int capacity)
  : skyline_(0), width_(FLT_MIN), height_(FLT_MIN) {}

SkylineContour::SkylineContour(const SkylineContour& prev) 
  : width_(prev.width_), height_(prev.height_), 
  bTreeInfo_(prev.bTreeInfo_) {
  if( prev.skyline_ ) {
    skyline_ = prev.skyline_->copy();
  }
  else{ 
    skyline_ = 0;
  }
  }

// build from ParquetFP's BTree
// consider rotation of macro cells. 
SkylineContour::SkylineContour(const BTree& bTree, bool isRotate) 
  : skyline_(0), width_(FLT_MIN), height_(FLT_MIN) {

  bTreeInfo_.reserve(bTree.NUM_BLOCKS);
  for(int i=0; i<bTree.NUM_BLOCKS; i++) {
    // left, right, height, index

    if( !isRotate ) {
      InsertBtreeNode( 
          bTree.xloc(i), 
          bTree.xloc(i) + bTree.width(i),
          bTree.yloc(i) + bTree.height(i), i);
    }
    else {
      InsertBtreeNode( 
          bTree.yloc(i), 
          bTree.yloc(i) + bTree.height(i),
          bTree.xloc(i) + bTree.width(i), i);
    
    } 
  }
  EvaluateContour();
}

bool SkylineContour::operator=(const SkylineContour& prev) {
  width_ = prev.width_;
  height_ = prev.height_;
  bTreeInfo_ = prev.bTreeInfo_;
  if( prev.skyline_ ) {
    skyline_ = prev.skyline_->copy();
  }
  else{ 
    skyline_ = 0;
  }
  return true;
}
SkylineContour::~SkylineContour() {
  Clear();
}

void SkylineContour::Clear() {
  if( skyline_ ) {
    delete skyline_;
    skyline_ = 0;
  }
  width_ = height_ = 0;
  std::vector<BTreeNode>().swap(bTreeInfo_);
}


void SkylineContour::InsertBtreeNode( 
    float _left, float _right, float _height, 
    int _idx) {
//  cout << _left << " " << _right << " " << _height << " " << _idx << endl;
//  cout << "bTreeInfoSize: " << bTreeInfo_.size() << endl;
  bTreeInfo_.push_back( SkylineContour::BTreeNode(_left, _right, _height, _idx) );
}

// Evaluate Contour
void SkylineContour::EvaluateContour() {
  if( skyline_ ) {
    delete skyline_;
  }
  skyline_ = findSkyline( bTreeInfo_, 0, bTreeInfo_.size()-1);
  height_ = 0;
  for(int i=0; i<skyline_->count(); i++) {
    height_ = (height_ < skyline_->arr[i].ht)? skyline_->arr[i].ht : height_;
  }

  width_ = skyline_->arr[skyline_->count()-1].left;
}


// traverse the Skyline Contour and get maximum heights 
// between [left, right] 
float SkylineContour::GetHeight(float left, float right) {
  if( !skyline_ ) {
    return 0;
  }

  float height = 0;

  for(int i=0; i<skyline_->count()-1; i++) {
    float lx = fmax( left, skyline_->arr[i].left );
    float ux = fmin( right, skyline_->arr[i+1].left );
    if( lx >= ux ) {
      continue; 
    }
    cout << "seg: " << lx << " " << ux << " " << skyline_->arr[i].ht << endl;
    height = (height < skyline_->arr[i].ht)? skyline_->arr[i].ht : height;
  }

  cout << "return height: " << height << endl;
  return height;
}

std::pair<float, int> SkylineContour::GetHeightWithIdx(float left, float right) {
  if( !skyline_ ) {
    return std::make_pair(0, -1);
  }

  float height = 0;
  int idx = -1;

  for(int i=0; i<skyline_->count()-1; i++) {
    float lx = fmax( left, skyline_->arr[i].left );
    float ux = fmin( right, skyline_->arr[i+1].left );
    if( lx >= ux ) {
      continue; 
    }
    cout << "seg: " << lx << " " << ux << " " << skyline_->arr[i].ht << endl;
    if( height < skyline_->arr[i].ht ) {
      height = skyline_->arr[i].ht;
      idx = skyline_->arr[i].index; 
    }
  }

  cout << "return height: " << height << endl;
  return std::make_pair( height, idx );
}

// extract ContourArea
float SkylineContour::GetContourArea() {
  float area = 0.0f;
  for(int i=0; i<skyline_->count()-1; i++) {
  
    float prevPoint = skyline_->arr[i].left;
    float nextPoint = skyline_->arr[i+1].left;
//    cout << prevPoint << " - " << nextPoint << " : " << skyline_->arr[i].ht << " " 
//      << (nextPoint - prevPoint) * skyline_->arr[i].ht << endl;
    area += (nextPoint - prevPoint) * skyline_->arr[i].ht;
  }

//  cout << "final: " << area << endl;

  return area;
}

/*
int GetVal (int min, int max) {
  return rand() % (max-min) + min;
}

float GetRandFloat () {
  return 1000.0f * static_cast <float> (rand()) / static_cast <float> (RAND_MAX); 
}

// drive program 
int main() 
{ 
  int num = 200000;
  Block* arr = NULL;
  arr = new Block[num];

  for(int i=0; i<num; i++) {
//    int x1 = GetVal( 0, 20 );
//    int x3 = GetVal( 1, 30 ); 
//    int h = GetVal( 0, 50 );
    float x1 = GetRandFloat( );
    float x3 = GetRandFloat( ); 
    float h = GetRandFloat( );

    if( x3 < x1 ) {
      swap(x1, x3);
    }
    Block tmp = {x1, h, x3};
    arr[i] = tmp; 
  }

  cout << "building Info" << endl;
  for(int i=0; i<num; i++) {
    cout << i << " Left:" << arr[i].left << " Height:" << arr[i].ht << " Right:" << arr[i].right << endl;
  }

  // Find skyline for given buildings and print the skyline 
  SkylineContour *ptr = findSkylineContour(arr, 0, num-1); 
  cout << " SkylineContour for given buildings is \n"; 
  ptr->print();
  delete ptr;

  return 0; 
}
*/
