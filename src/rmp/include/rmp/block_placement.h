#ifndef BLOCK_PLACEMENT_H_
#define BLOCK_PLACEMENT_H_

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <random>

#include "rmp/shape_engine.h"

namespace block_placement {
    // definition of blocks:  include semi-soft blocks and soft blocks
    class Block {
        private:
            float width_ = 0.0;
            float height_ = 0.0;
            float area_ = 0.0;
            float x_ = 0.0;  // lx
            float y_ = 0.0;  // ly
            std::string name_;
            bool is_soft_ = true;
            int num_macro_ = 0;
        
            std::vector<std::pair<float, float> > aspect_ratio_;
            std::vector<std::pair<float, float> > width_limit_; // This is in increasing order
            std::vector<std::pair<float, float> > height_limit_; // This is in decreasing order

            std::mt19937* generator_;
            std::uniform_real_distribution<float>* distribution_;

        public:
            Block() {   };

            Block(std::string name, float area, int num_macro, 
                  std::vector<std::pair<float, float> > aspect_ratio) {
                name_ = name;
                area_ = area;
                num_macro_ = num_macro;
                if(num_macro_ == 0)
                    is_soft_ = true;
                else
                    is_soft_ = false;

                aspect_ratio_ = aspect_ratio;

                // sort the aspect ratio according to the 1st element of the pair in ascending order
                // And we assume the aspect_ratio[i].first <= aspect_ratio[i].second
                std::sort(aspect_ratio_.begin(), aspect_ratio_.end());
                for(int i = 0; i < aspect_ratio_.size(); i++) {
                    float height_low = std::sqrt(area_ * aspect_ratio_[i].first);
                    float width_high = area_ / height_low;

                    float height_high = std::sqrt(area_ * aspect_ratio_[i].second);
                    float width_low = area_ / height_high;
                    
                    // height_limit_ is sorted in non-decreasing order
                    height_limit_.push_back(std::pair<float, float>(height_low, height_high));

                    // width_limit_ is sorted in non-increasing order
                    width_limit_.push_back(std::pair<float, float>(width_high, width_low));
                }
                // Initialize shape (width_, height_) randomly
                //ChooseAspectRatioRandom();
            }
            
            
            Block(const Block& block) {
                this->width_ = block.width_;
                this->height_ = block.height_;
                this->area_ = block.area_;
                this->x_ = block.x_;
                this->y_ = block.y_;
                this->name_ = block.name_;
                this->is_soft_ = block.is_soft_;
                this->num_macro_ = block.num_macro_;
                this->aspect_ratio_ = block.aspect_ratio_;
                this->width_limit_ = block.width_limit_;
                this->height_limit_ = block.height_limit_;
                this->generator_ = block.generator_;
                this->distribution_ = block.distribution_;
            }

            void operator=(const Block& block) {
                this->width_ = block.width_;
                this->height_ = block.height_;
                this->area_ = block.area_;
                this->x_ = block.x_;
                this->y_ = block.y_;
                this->name_ = block.name_;
                this->is_soft_ = block.is_soft_;
                this->num_macro_ = block.num_macro_;
                this->aspect_ratio_ = block.aspect_ratio_;
                this->width_limit_ = block.width_limit_;
                this->height_limit_ = block.height_limit_;
                this->generator_ = block.generator_;
                this->distribution_ = block.distribution_;
            }

            // Accesor
            bool IsSoft() { return is_soft_; }
            std::string GetName()  { return name_; }
            float GetX() { return x_; }
            float GetY() { return y_; }
            float GetWidth() { return width_; }
            float GetHeight() { return height_; }
            float GetArea() { return area_; }
            float GetAspectRatio() { return height_ / width_; }
            int GetNumMacro() { return num_macro_; }

            void SpecifyX(float x) { x_ = x; }
            void SpecifyY(float y) { y_ = y; }
            void SpecifyAspectRatio(float aspect_ratio) {
                height_ = std::sqrt(area_ * aspect_ratio);
                width_ = area_ / height_;
            }

            void SpecifyRandom(std::mt19937& generator, std::uniform_real_distribution<float>& distribution) {
                generator_ = &generator;
                distribution_ = &distribution;
                ChooseAspectRatioRandom();
            }
            
            void ChangeWidth(float width) {
                if(is_soft_ == false)
                    return;
                
                if(width >= width_limit_[0].first) {
                    width_ = width_limit_[0].first;
                    height_ = area_ / width_;
                } else if(width <= width_limit_[width_limit_.size() - 1].second) {
                    width_ = width_limit_[width_limit_.size() - 1].second;
                    height_ = area_ / width_;
                } else {
                    std::vector<std::pair<float, float> >::iterator  vec_it = width_limit_.begin();
                    while(vec_it->second > width)
                        vec_it++;

                    if(width <= vec_it->first) {
                        width_ = width;
                        height_ = area_ / width_;
                    } else {
                        float width_low = vec_it->first;
                        vec_it--;
                        float width_high = vec_it->second;
                        if(width - width_low > width_high - width)
                            width_ = width_high;
                        else
                            width_ = width_low;
                        
                        height_ = area_ / width_;
                    }
                }
            }


            void ChangeHeight(float height) {
                if(is_soft_ == false)
                    return;

                if(height <= height_limit_[0].first) {
                    height_ = height_limit_[0].first;
                    width_ = area_ / height_;
                } else if(height >= height_limit_[height_limit_.size() - 1].second) {
                    height_ = height_limit_[height_limit_.size() - 1].second;
                    width_ = area_ / height_;
                } else {
                    std::vector<std::pair<float, float> >::iterator vec_it = height_limit_.begin();
                    while(vec_it->second < height)
                        vec_it++;
 
                    if(height >= vec_it->first) {
                        height_ = height;
                        width_ = area_ / height_;
                    } else {
                        float height_high = vec_it->first;
                        vec_it--;
                        float height_low = vec_it->second;
                        if(height - height_low > height_high - height)
                            height_ = height_high;
                        else
                            height_ = height_low;

                        width_ = area_ / height_;
                    }
                }
            }

        
            bool IsResize() {
                if(num_macro_ > 0 && aspect_ratio_.size() == 1)
                    return false;
                else
                    return true;
            }

            void ResizeHardBlock() {
                if(num_macro_ == 0 || (num_macro_  > 0 && aspect_ratio_.size() == 1)) {
                    return;
                } else {
                    int index1 = (int)(floor((*distribution_)(*generator_) * aspect_ratio_.size()));
                    float ar = aspect_ratio_[index1].first;
                    float temp_ar = height_ / width_;
                    while(abs(ar - temp_ar) / ar < 0.01) {
                        index1 = (int)(floor((*distribution_)(*generator_) * aspect_ratio_.size()));
                        ar = aspect_ratio_[index1].first;
                        temp_ar = height_ / width_;
                    }
                    
                    height_ = std::sqrt(area_ * ar);
                    width_ = area_ / height_;
                }
            }


            void ChooseAspectRatioRandom() {
                float ar = 0.0;
                int index1 = (int)(floor((*distribution_)(*generator_) * aspect_ratio_.size()));
                
                float ar_low = aspect_ratio_[index1].first;
                float ar_high = aspect_ratio_[index1].second;
    
                if(ar_low == ar_high) {
                    ar = ar_low;
                } else {
                    float num = (*distribution_)(*generator_);
                    ar = ar_low + (ar_high - ar_low) * num;
                }
            
                height_ = std::sqrt(area_ * ar);
                width_ = area_ / height_;
            }
            
            void RemoveSoftBlock() {
                if(num_macro_ == 0) {
                    width_ = 0.0;
                    height_ = 0.0;
                }
            }
        
            void ShrinkSoftBlock(float width_factor, float height_factor) {
                //if(num_macro_ == 0) {
                    std::cout << "name:   " << name_ << "   ";
                    std::cout << "pre_width:  " << width_ << "   ";
                    std::cout << "pre_height:  " << height_ << "   ";
                    width_ = width_ * width_factor;
                    height_ = height_ * height_factor;
                    //area_ = width_ * height_;
                    area_ = width_ * height_;
                    std::cout << "width:   " << width_ << "   ";
                    std::cout << "height:  " << height_ << "   ";
                    std::cout << std::endl;
                //}
            }

    };


    struct Net {
        int weight_ = 0;
        std::vector<std::string> blocks_;
        std::vector<std::string> terminals_;

        Net(int weight, std::vector<std::string> blocks, 
                std::vector<std::string> terminals) {
            weight_ = weight;
            blocks_ = blocks;
            terminals_ = terminals;
        }
    };
    

    struct Region {
        float lx_ = 0.0;
        float ly_ = 0.0;
        float ux_ = 0.0;
        float uy_ = 0.0;

        Region(float lx, float ly, float ux, float uy) {
            lx_ = lx;
            ly_ = ly;
            ux_ = ux;
            uy_ = uy;
        }
    };

    class SimulatedAnnealingCore {
        private:
            // These parameters are related to fastSA
            float init_prob_ = 0.95;
            float rej_ratio_ = 0.95;
            int max_num_step_ = 1000;
            int k_;
            float c_;
            int perturb_per_step_ = 60;
            float init_T_;
            
            float outline_width_;
            float outline_height_;
            
            // learning rate for dynamic weight
            float learning_rate_ = 0.01;

            // shrink_factor for dynamic weight
            float shrink_factor_ = 0.995;
            float shrink_freq_ = 0.01;

            float height_;
            float width_;
            float area_;
            float wirelength_;
            float outline_penalty_;
            float boundary_penalty_;
            float macro_blockage_penalty_;

            float pre_height_;
            float pre_width_;
            float pre_area_;
            float pre_wirelength_;
            float pre_outline_penalty_;
            float pre_boundary_penalty_;
            float pre_macro_blockage_penalty_;

            float norm_area_;
            float norm_wirelength_;
            float norm_outline_penalty_;
            float norm_boundary_penalty_;
            float norm_macro_blockage_penalty_;

            // These parameters are related to cost function
            float alpha_; // weight for area
            float beta_; // weight for wirelength
            float gamma_; // weight for outline penalty
            float boundary_weight_; // weight for boundary penalty
            float macro_blockage_weight_; // weight for macro blockage weight


            float alpha_base_; 
            float beta_base_;
            float gamma_base_;
            float boundary_weight_base_;
            float macro_blockage_weight_base_;


            // These parameters are related to action probabilities
            float resize_prob_ = 0.4;
            float pos_swap_prob_ = 0.2;
            float neg_swap_prob_ = 0.2;
            float double_swap_prob_ = 0.2;
           
            std::unordered_map<std::string, int> block_map_;
            std::unordered_map<std::string, std::pair<float, float> > terminal_position_;
            std::vector<Net*> nets_;
            std::vector<Region*> regions_;
            
            std::vector<Block> blocks_;
            std::vector<Block> pre_blocks_;
            
            int action_id_ = -1;
            int block_id_ = -1;


            std::vector<int> pos_seq_;
            std::vector<int> neg_seq_;
            std::vector<int> pre_pos_seq_;
            std::vector<int> pre_neg_seq_;

            std::mt19937 generator_;
            std::uniform_real_distribution<float> distribution_;

            void PackFloorplan();
            void Resize();
            void SingleSwap(bool flag); // true for pos_seq and false for neg_seq
            void DoubleSwap();
            void Perturb();
            void Restore();
            void CalculateOutlinePenalty();
            void CalculateBoundaryPenalty();
            void CalculateMacroBlockagePenalty();
            void CalculateWirelength();
            float NormCost(float area, float wirelength, float outline_penalty,
                         float boundary_penalty, float macro_blockage_penalty);

            void UpdateWeight(float avg_area, float avg_wirelength, float avg_outline_penalty,       
                         float avg_boundary_penalty, float avg_macro_blockage_penalty);

        public:
            // Constructor
            SimulatedAnnealingCore(float outline_width, float outline_height, 
                std::vector<Block>& blocks, std::vector<Net*>& nets, std::vector<Region*>& regions,
                std::unordered_map<std::string, std::pair<float, float> > terminal_position,
                float alpha, float beta, float gamma, float boundary_weight, float macro_blockage_weight,
                float resize_prob, float pos_swap_prob, float neg_swap_prob, float double_swap_prob,
                float init_prob, float rej_ratio, int max_num_step, int k, float c, int perturb_per_step,
                float learning_rate, float shrink_factor, float shrink_freq,
                unsigned seed = 0) {
                outline_width_ = outline_width;
                outline_height_ = outline_height;
              
                learning_rate_ = learning_rate;
                shrink_factor_ = shrink_factor;
                shrink_freq_ = shrink_freq;

                alpha_ = alpha;
                beta_ = beta;
                gamma_ = gamma;
                boundary_weight_ = boundary_weight;
                macro_blockage_weight_ = macro_blockage_weight;


                alpha_base_ = alpha_;
                beta_base_ = beta_;
                gamma_base_ = gamma_;
                boundary_weight_base_ = boundary_weight_;
                macro_blockage_weight_base_ = macro_blockage_weight_;


                resize_prob_ = resize_prob;
                pos_swap_prob_ = resize_prob_ + pos_swap_prob;
                neg_swap_prob_ = pos_swap_prob_ + neg_swap_prob;
                double_swap_prob_ = neg_swap_prob_ +  double_swap_prob;

                init_prob_ = init_prob;
                rej_ratio_ = rej_ratio;
                max_num_step_ = max_num_step;
                k_ = k;
                c_ = c;
                perturb_per_step_ = perturb_per_step;
                
                std::mt19937 randGen(seed);
                generator_ = randGen;
 
                std::uniform_real_distribution<float> distribution(0.0,1.0);
                distribution_ = distribution;

                nets_ = nets;
                regions_ = regions;
                terminal_position_ = terminal_position;
                
                for(int i = 0; i < blocks.size(); i++) {
                    pos_seq_.push_back(i);
                    neg_seq_.push_back(i);

                    pre_pos_seq_.push_back(i);
                    pre_neg_seq_.push_back(i);
                }

                blocks_ = blocks;
                for(int i = 0; i < blocks_.size(); i++) {
                    blocks_[i].SpecifyRandom(generator_, distribution_);
                    block_map_.insert(std::pair<std::string, int>(blocks_[i].GetName(), i));
                }
                
                pre_blocks_ = blocks_;
                
                //for(int i = 0; i < blocks_.size(); i++) {
                //    std::cout << "name:   " << blocks_[i].GetName() << "   ";
                //    std::cout << "width:  " << blocks_[i].GetWidth() << "   ";
                //    std::cout << "height:  " << blocks_[i].GetHeight() << "   ";
                //    std::cout << std::endl;
                //}
                    
            
            }
           
            void FastSA();

            void Initialize();

            void Initialize(float init_T, float norm_area, float norm_wirelength, float norm_outline_penalty,
                    float norm_boundary_penalty, float norm_macro_blockage_penalty) {
                init_T_ = init_T;
                norm_area_ = norm_area;
                norm_wirelength_ = norm_wirelength;
                norm_outline_penalty_ = norm_outline_penalty;
                norm_boundary_penalty_ = norm_boundary_penalty;
                norm_macro_blockage_penalty_ = norm_macro_blockage_penalty;
            }

            void SpecifySeq(std::vector<int> pos_seq, std::vector<int> neg_seq) {
                pos_seq_ = pos_seq;
                neg_seq_ = neg_seq;
                pre_pos_seq_ = pos_seq;
                pre_neg_seq_ = neg_seq;
                PackFloorplan();
                CalculateWirelength();
                CalculateOutlinePenalty();
                CalculateBoundaryPenalty();
                CalculateMacroBlockagePenalty();
            }

            float GetInitT()  { return init_T_; }
            float GetNormArea() { return norm_area_; }
            float GetNormWirelength() { return norm_wirelength_; }
            float GetNormOutlinePenalty() { return norm_outline_penalty_; }
            float GetNormBoundaryPenalty() { return norm_boundary_penalty_; }
            float GetNormMacroBlockagePenalty() { return norm_macro_blockage_penalty_; }

            float GetCost() {
                //PackFloorplan();
                //CalculateWirelength();
                //CalculateOutlinePenalty();
                //CalculateBoundaryPenalty();
                //CalculateMacroBlockagePenalty();
                return NormCost(area_, wirelength_,  outline_penalty_, boundary_penalty_, macro_blockage_penalty_); 
            }

            float GetWidth()  { return width_; }
            float GetHeight() { return height_; }
            float GetArea() { return area_; }
            float GetWirelength() { return wirelength_; }
            float GetOutlinePenalty() { return outline_penalty_; }
            float GetBoundaryPenalty() { return boundary_penalty_; }
            float GetMacroBlockagePenalty() { return macro_blockage_penalty_; }
            std::vector<Block> GetBlocks() { return blocks_; }
            std::vector<int> GetPosSeq() { return pos_seq_; }
            std::vector<int> GetNegSeq() { return neg_seq_; }
            
            void ShrinkBlocks();    
            bool FitFloorplan();


            bool IsFeasible() {
                float tolerance = 0.001;
                if(width_ <= outline_width_ * (1 + tolerance) &&  height_ <= outline_height_ * (1 + tolerance))
                    return true;
                else
                    return false;
            }
    };


    // wrapper for run function of SimulatedAnnealingCore
    void Run(SimulatedAnnealingCore* sa);

    void ParseNetFile(std::vector<Net*>& nets,  std::unordered_map<std::string, std::pair<float, float> >& terminal_position,
        const char* net_file);

    
    void ParseRegionFile(std::vector<Region*>& regions, const char* region_file);


    std::vector<Block> Floorplan(std::vector<shape_engine::Cluster*> clusters, float outline_width, float outline_height, 
        const char* net_file, const char* region_file, int num_level, int num_worker, float heat_rate,
        float alpha, float beta, float gamma, float boundary_weight, float macro_blockage_weight, 
        float resize_prob, float pos_swap_prob, float neg_swap_prob, float double_swap_prob, 
        float init_prob, float rej_ratio, int max_num_step, int k, float c, int perturb_per_step,
        float learning_rate, float shrink_factor, float shrink_freq,
        unsigned seed = 0);




}
#endif
