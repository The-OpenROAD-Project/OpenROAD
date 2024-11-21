// File: RCProgressTracker.hpp
#pragma once

#include <cmath>
#include <iostream>
#include <string>

namespace rcx {

class ExtProgressTracker
{
 public:
  // Constructor with optional custom progress interval (default 5%)
  explicit ExtProgressTracker(int totalWires, float progressInterval = 5.0f)
      : totalWireCount_(totalWires),
        extractedWireCount_(0),
        previousProgressPercent_(0.0f),
        progressInterval_(progressInterval),
        wire_extracted_progress_count(500000),
        enabled_(true)
  {
  }

  // Update progress and print if threshold reached
  bool updateProgress(int newlyExtractedCount = 1)
  {
    if (!enabled_ || totalWireCount_ <= 0) {
      return false;
    }
    extractedWireCount_ += newlyExtractedCount;

    float currentPercent = calculateCurrentPercent();

    if (shouldPrintProgress(currentPercent)) {
      printProgress(currentPercent);
      previousProgressPercent_ = currentPercent;
      return true;
    }

    return false;
  }

  // Getter methods
  int getTotalWireCount() const { return totalWireCount_; }
  int getExtractedCount() const { return extractedWireCount_; }
  float getProgressPercent() const { return calculateCurrentPercent(); }

  // Enable/disable progress reporting
  void enableReporting(bool enable) { enabled_ = enable; }

  // Reset progress
  void reset()
  {
    extractedWireCount_ = 0;
    previousProgressPercent_ = 0.0f;
  }

  // Set custom progress message format
  void setProgressFormat(const std::string& format)
  {
    progressFormat_ = format;
  }

 private:
  int totalWireCount_;
  int extractedWireCount_;
  float previousProgressPercent_;
  float progressInterval_;
  bool enabled_;
  int wire_extracted_progress_count;
  std::string progressFormat_
      = "{:.1f}% completion -- {} wires have been extracted\n";

  float calculateCurrentPercent() const
  {
    return ceil(100.0f * static_cast<float>(extractedWireCount_)
                / totalWireCount_);
  }

  bool shouldPrintProgress(float currentPercent) const
  {
    return enabled_ && extractedWireCount_ > 0
           && extractedWireCount_ % wire_extracted_progress_count == 0
           && (currentPercent - previousProgressPercent_ >= progressInterval_);
  }

  void printProgress(float currentPercent) const
  {
    fprintf(stdout,
            "%3d%% completion -- %6d wires have been extracted\n",
            static_cast<int>(currentPercent),
            extractedWireCount_);
  }
};

// Helper class for periodic progress updates
class ScopedProgressUpdate
{
 public:
  ScopedProgressUpdate(ExtProgressTracker& tracker, int updateCount = 1)
      : tracker_(tracker), updateCount_(updateCount)
  {
  }

  ~ScopedProgressUpdate() { tracker_.updateProgress(updateCount_); }

 private:
  ExtProgressTracker& tracker_;
  int updateCount_;
};

}  // namespace rcx
