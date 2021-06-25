#ifndef SHAPE_ENGINE_H_
#define SHAPE_ENGINE_H_

#include "utl/Logger.h"

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <random>
#include <algorithm>
#include <map>
#include <fstream>


namespace shape_engine {
    // definition of Orientation
    enum Orientation {
        R0,
        R90,
        R180,
        R270,
        MX,
        MY,
        MX90,
        MY90
    };
        

    // definition of hard macro
    class Macro {
        static std::map<Orientation, Orientation> FLIP_X_TABLE;
        static std::map<Orientation, Orientation> FLIP_Y_TABLE;

        private:
            float width_ = 0.0;
            float height_ = 0.0;
            float area_ = 0.0;
            float x_ = 0.0;
            float y_ = 0.0;
            float pin_x_ = 0.0;
            float pin_y_ = 0.0;
            Orientation orientation_ = R0;
            std::string name_; 



            std::string OrientationToString(Orientation orientation) {
                switch (orientation)
                {
                    case R0 :    return std::string("R0");
                    case R180 :  return std::string("R180");
                    case R90 :   return std::string("R90");
                    case R270 :  return std::string("R270");
                    case MY :    return std::string("MY");
                    case MX :    return std::string("MX");
                    case MX90 :  return std::string("MX90");
                    case MY90 :  return std::string("MY90");
                    default :    return std::string("Unknown");
                }
            }

        public:
            // Constructors
            Macro() {  };

            Macro(std::string name, float width, float height) {
                name_ = name;
                width_ = width;
                height_ = height;
                area_ = width_ * height_;
            }

            Macro(const Macro& macro) {
                this->name_ = macro.name_;
                this->width_ = macro.width_;
                this->height_ = macro.height_;
                this->x_ = macro.x_;
                this->y_ = macro.y_;
                this->area_ = macro.area_;
                this->pin_x_ = macro.pin_x_;
                this->pin_y_ = macro.pin_y_;
                this->orientation_ = macro.orientation_;
            }

            void operator=(const Macro& macro) {
                this->name_ = macro.name_;
                this->width_ = macro.width_;
                this->height_ = macro.height_;
                this->x_ = macro.x_;
                this->y_ = macro.y_;
                this->area_ = macro.area_;
                this->pin_x_ = macro.pin_x_;
                this->pin_y_ = macro.pin_y_;
                this->orientation_ = macro.orientation_;
            }
           
            // overload the operator <
            bool operator<(const Macro& macro) const {
                if(width_ != macro.width_)
                    return width_ < macro.width_;

                return height_ < macro.height_;
            }

            bool operator==(const Macro& macro) const {
                if(width_ == macro.width_ && height_ == macro.height_)
                    return true;
                else
                    return false;
            }


            // accessor
            float GetWidth()  {  return width_; }
            float GetHeight()  {    return height_; }
            float GetX()  {  return x_; }
            float GetY()  {  return y_; }
            float GetArea() {  return area_; }
            float GetPinX() {  return pin_x_; }
            float GetPinY() {  return pin_y_; }
            std::string GetOrientation() {  
                return OrientationToString(orientation_);  
            }

            std::string GetName() { return name_; }

            void SpecifyX(float x) { x_ = x; }
            void SpecifyY(float y) { y_ = y; }
            void SpecifyPinPosition(float pin_x, float pin_y) {
                pin_x_ = pin_x;
                pin_y_ = pin_y;
            }

            // operation
            void Flip(bool axis) {
                if(axis == true) {
                    // FLIP Y
                    orientation_ = FLIP_Y_TABLE[orientation_];
                    pin_x_ = width_ -  pin_x_;
                } else {
                    // FLIP X
                    orientation_ = FLIP_X_TABLE[orientation_];
                    pin_y_ = height_ - pin_y_;
                }
            }
    };

    
    // definition of clusters:  include std cell clusters and hard macro clusters
    class Cluster {
        private:
            std::string name_;
            float area_ = 0.0;
            std::vector<std::pair<float, float> > aspect_ratio_;
            std::vector<Macro> macros_;
            float x_ = 0.0;
            float y_ = 0.0;
            float width_ = 0.0;
            float height_ = 0.0;

        public:
            Cluster() {   };
            Cluster(std::string name) { name_ = name; }
            
            std::string GetName()  { return name_; }
            float GetArea() { return area_; }
            std::vector<std::pair<float, float> > GetAspectRatio() { return aspect_ratio_; }
            std::vector<Macro> GetMacros() { return macros_; }
            int GetNumMacro() { return macros_.size(); }

            void SpecifyPos(float x, float y) { 
                x_ = x;
                y_ = y;
            }

            void SpecifyFootprint(float width, float height) {
                width_ = width;
                height_ = height;
            }

            float GetX()  { return x_; }
            float GetY()  { return y_; }
            float GetWidth()  { return width_; }
            float GetHeight() { return height_; }


            void SpecifyArea(float area)  { area_ = area; }
            void AddArea(float area) { area_ += area; }
            void SpecifyAspectRatio(std::vector<std::pair<float, float> > aspect_ratio) {
                aspect_ratio_ = aspect_ratio;
            }

            void AddAspectRatio(std::pair<float, float> ar) {
                aspect_ratio_.push_back(ar);
            }

           
            void AddMacro(Macro macro) { macros_.push_back(macro); }
            void SpecifyMacros(std::vector<Macro> macros) { macros_ = macros; }


            bool operator==(const Cluster& cluster) const {
                return macros_ == cluster.macros_;
            }

            void SortMacro() {
                std::sort(macros_.begin(), macros_.end());
            }
    };








    class SimulatedAnnealingCore {
        private:
            float init_prob_ = 0.95;
            float rej_ratio_ = 0.95;
            int max_num_step_ = 1000;
            int k_;
            float c_;
            int perturb_per_step_ = 60;
            float alpha_ = 0.5;

            float outline_width_ = 0.0;
            float outline_height_ = 0.0;

            float height_ = 0.0;
            float width_ = 0.0;
            float area_ = 0.0;
    
            float pre_height_ = 0.0;
            float pre_width_ = 0.0;
            float pre_area_ = 0.0;
    

            float norm_blockage_ = 0.0;
            float norm_area_ = 0.0;
            float init_T_ = 0.0;
           
            int action_id_ = -1;


            std::vector<Macro*> macros_;

            std::vector<int> pos_seq_;
            std::vector<int> neg_seq_;
            std::vector<int> pre_pos_seq_;
            std::vector<int> pre_neg_seq_;

            float pos_swap_prob_ = 0.4;
            float neg_swap_prob_ = 0.8; // actually prob = 0.4
            float double_swap_prob_ = 1.0; // actually prob = 0.2
            
            std::mt19937 generator_;
            std::uniform_real_distribution<float> distribution_;
            
            void PackFloorplan();
            void SingleSwap(bool flag); // true for pos swap and false for neg swap
            void DoubleSwap();
            void Perturb();
            void Restore();
            void Initialize();
            float CalculateBlockage();
            float NormCost(float area, float blockage_cost);
            void FastSA();


        public:
            // constructor
            SimulatedAnnealingCore(std::vector<Macro> macros, float outline_width, float outline_height,
                                   float init_prob = 0.95, float rej_ratio = 0.95, int max_num_step = 1000,
                                   int k = 100, float c = 100, int perturb_per_step = 60, float alpha = 0.5,
                                   float pos_swap_prob = 0.4, float neg_swap_prob = 0.4, float double_swap_prob = 0.2,
                                   unsigned seed = 0
                                  )
            {
                outline_width_ = outline_width;
                outline_height_  = outline_height;

                init_prob_ = init_prob;
                rej_ratio_ = rej_ratio;
                max_num_step_ = max_num_step;
                k_ = k;
                c_ = c;
                perturb_per_step_ = perturb_per_step;
                alpha_ = alpha;

                pos_swap_prob_ = pos_swap_prob;
                neg_swap_prob_ = pos_swap_prob_ + neg_swap_prob;
                double_swap_prob_ = neg_swap_prob_ + double_swap_prob;
                
                for(size_t i = 0; i < macros.size(); i++) {
                    pos_seq_.push_back(i);
                    neg_seq_.push_back(i);
                   
                    pre_pos_seq_.push_back(i);
                    pre_neg_seq_.push_back(i);

                    Macro* macro = new Macro(macros[i]);
                    macros_.push_back(macro);
                }

                std::mt19937 randGen(seed);
                generator_ = randGen;
                std::uniform_real_distribution<float> distribution(0.0,1.0);
                distribution_ = distribution;
                
                // Initialize init_T_, norm_blockage_, norm_area_
                Initialize();
            }
            
            void Run() {
                FastSA();
            }
            
            float GetWidth() {  return width_; }
            float GetHeight() { return height_; }
            float GetArea() { 
                return area_;
            }

            void WriteFloorplan(std::string file_name) {
                std::ofstream file;
                file.open(file_name);
                for(size_t i = 0; i < macros_.size(); i++) {
                    file << macros_[i]->GetX() << "   ";
                    file << macros_[i]->GetY() << "   ";
                    file << macros_[i]->GetX() + macros_[i]->GetWidth() << "   ";
                    file << macros_[i]->GetY() + macros_[i]->GetHeight() << "   ";
                    file << std::endl;
                }    
                
                file.close();
            }

    };

    // wrapper for run function of SimulatedAnnealingCore
    void Run(SimulatedAnnealingCore* sa);


    // Macro Tile Engine
    std::vector<std::pair<float, float> > TileMacro(std::vector<Macro> macros, std::string cluster_name, float& final_area, 
            float outline_width, float outline_height, int num_thread = 10, int num_run = 20, unsigned seed = 0);


    // Parse Block File
    void ParseBlockFile(std::vector<Cluster*>& clusters, const char* file_name,
        float &outline_width, float &outline_height, float& outline_lx, float& outline_ly,
        utl::Logger* logger,
        float dead_space, float halo_width);


    // Top level function : Shape Engine
    std::vector<Cluster*> ShapeEngine(float& outline_width,  float& outline_height,
        float& outline_lx, float& outline_ly,
        float min_aspect_ratio, float dead_space, float halo_width,
        utl::Logger* logger,
        const char* block_file, int num_thread = 10, int num_run = 20, unsigned seed = 0
        );



}











#endif
