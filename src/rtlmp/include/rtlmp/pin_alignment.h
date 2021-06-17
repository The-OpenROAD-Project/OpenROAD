#ifndef PIN_ALIGNMENT_H_
#define PIN_ALIGNMENT_H_

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <random>
#include <algorithm>

#include "rtlmp/block_placement.h"
#include "rtlmp/shape_engine.h"


namespace pin_alignment {

    class SimulatedAnnealingCore {
        private:
            float init_prob_ = 0.95;
            float rej_ratio_ = 0.95;
            int max_num_step_ = 1000;
            int k_;
            float c_;
            int perturb_per_step_ = 60;
            float alpha_ = 0.4;
            float beta_ = 0.3;
            float gamma_ = 0.3;
        
            float outline_width_ = 0.0;
            float outline_height_ = 0.0;

            float height_ = 0.0;
            float width_ = 0.0;
            float area_ = 0.0;
            float wirelength_ = 0.0;
            float outline_penalty_ = 0.0;
        
        
            float pre_height_ = 0.0;
            float pre_width_ = 0.0;
            float pre_area_ = 0.0;
            float pre_wirelength_ = 0.0;
            float pre_outline_penalty_ = 0.0;


            float norm_area_ = 0.0;
            float norm_wirelength_ = 0.0;
            float norm_outline_penalty_ = 0.0;
            float init_T_ = 0.0;
           
            int action_id_ = -1;
            bool flip_flag_ = true;
            std::vector<shape_engine::Macro> macros_;
            std::vector<block_placement::Net*> nets_;
            std::unordered_map<std::string, std::pair<float, float> > terminal_position_;
            std::unordered_map<std::string, int> macro_map_;

            std::vector<int> pos_seq_;
            std::vector<int> neg_seq_;
            std::vector<int> pre_pos_seq_;
            std::vector<int> pre_neg_seq_;

            float flip_prob_ = 0.2;
            float pos_swap_prob_ = 0.5; // actually prob = 0.3
            float neg_swap_prob_ = 0.8; // actually prob = 0.3
            float double_swap_prob_ = 1.0; // actually prob = 0.2
            
            std::mt19937 generator_;
            std::uniform_real_distribution<float> distribution_;
            
            void PackFloorplan();
            void SingleFlip();
            void SingleSwap(bool flag); // true for pos swap and false for neg swap
            void DoubleSwap();
            void Flip();
            void Perturb();
            void Restore();
            void Initialize();
            void CalculateWirelength();
            void CalculateOutlinePenalty();
            float NormCost(float area, float wirelength, float outline_penalty);
            void FastSA();


        public:
            // constructor
            SimulatedAnnealingCore(std::vector<shape_engine::Macro> macros, std::vector<block_placement::Net*>& nets,
                                   std::unordered_map<std::string, std::pair<float, float> >& terminal_position,
                                   float outline_width, float outline_height,
                                   float init_prob = 0.95, float rej_ratio = 0.95, int max_num_step = 1000,
                                   int k = 100, float c = 100, int perturb_per_step = 60, 
                                   float alpha = 0.4, float beta = 0.3, float gamma = 0.3, float flip_prob = 0.2,
                                   float pos_swap_prob = 0.3, float neg_swap_prob = 0.3, float double_swap_prob = 0.2,
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
                beta_ = beta;
                gamma_ = gamma;

                flip_prob_ = flip_prob;
                pos_swap_prob_ = flip_prob_ +  pos_swap_prob;
                neg_swap_prob_ = pos_swap_prob_ + neg_swap_prob;
                double_swap_prob_ = neg_swap_prob_ + double_swap_prob;

                nets_ = nets;
                terminal_position_ = terminal_position;
                
                for(int i = 0; i < macros.size(); i++) {
                    pos_seq_.push_back(i);
                    neg_seq_.push_back(i);
                   
                    pre_pos_seq_.push_back(i);
                    pre_neg_seq_.push_back(i);

                    macros_.push_back(shape_engine::Macro(macros[i]));
                    
                    macro_map_.insert(std::pair<std::string, int>(macros[i].GetName(), i));
                }

                std::mt19937 randGen(seed);
                generator_ = randGen;
                std::uniform_real_distribution<float> distribution(0.0,1.0);
                distribution_ = distribution;
               
                if(macros_.size() == 1) {
                    width_ = macros_[0].GetWidth();
                    height_ = macros_[0].GetHeight();
                    area_ = width_ * height_;
                } else {
                    // Initialize init_T_, norm_area_, norm_wirelength, norm_outline_penalty_
                    Initialize();
                }
            }
            
            void Run() {
                if(macros_.size() > 1)
                    FastSA();
                else
                    SingleFlip();
            }
            
            float GetWidth() {  return width_; }
            float GetHeight() { return height_; }
            float GetArea() { return area_; }
            float GetWirelength() { return wirelength_; };
            bool IsFeasible() {
                std::cout << "width_ " << width_ << "    ";
                std::cout << "height_  " << height_ << "    ";
                std::cout << "outline_width_  " << outline_width_ << "   ";
                std::cout << "outline_height_  " << outline_height_ << "   ";
                std::cout << std::endl;
                float tolerance = 0.01;
                if(width_ <= outline_width_ * (1 + tolerance) && height_ <= outline_height_ * (1 + tolerance))
                    return true;
                else
                    return false;
            }
            
            std::vector<shape_engine::Macro> GetMacros() { 
                std::cout << "outline_width:   " << outline_width_ << "  ";
                std::cout << "outline_height:  " << outline_height_ << "    ";
                std::cout << std::endl;
                
                for(int i = 0; i < macros_.size(); i++) {
                    std::cout << macros_[i].GetName() << "    ";
                    std::cout << macros_[i].GetX() << "    ";
                    std::cout << macros_[i].GetY() << "    ";
                    std::cout << macros_[i].GetX() + macros_[i].GetWidth() << "   ";
                    std::cout << macros_[i].GetY() + macros_[i].GetHeight() << "    ";
                    std::cout << std::endl;
                }
                std::cout << std::endl;
                
                return macros_; 
            
            }


    };

    // wrapper for run function of SimulatedAnnealingCore
    void Run(SimulatedAnnealingCore* sa);


    // Parse macro file
    void ParseMacroFile(std::vector<shape_engine::Macro>& macros, float halo_width, std::string file_name);

    // Pin Alignment Engine
    void PinAlignment(std::vector<shape_engine::Cluster*>& clusters, float halo_width,  int num_thread, int num_run, unsigned seed = 0);

}











#endif
