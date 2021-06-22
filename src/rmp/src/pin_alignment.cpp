#include "rmp/pin_alignment.h"

#include <vector>
#include <random>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <thread>
#include <fstream>
#include <cfloat>
#include <string>

#include "rmp/shape_engine.h"
#include "rmp/block_placement.h"
#include "rmp/util.h"



namespace pin_alignment {
    using std::unordered_map;
    using std::vector;
    using std::swap;
    using std::log;
    using std::exp;
    using std::pair;
    using std::pow;
    using std::sort;
    using std::fstream;
    using std::string;
    using std::ios;
    using std::getline;
    using std::stof;
    using std::cout;
    using std::endl;
    using std::thread;
    using std::abs;
    using std::max;
    using std::min;
    using std::to_string;

    using shape_engine::Cluster;
    using shape_engine::Macro;
    using block_placement::Net;

    void SimulatedAnnealingCore::PackFloorplan() {       
        for(int i = 0; i < macros_.size(); i++) {
            macros_[i].SpecifyX(0.0);
            macros_[i].SpecifyY(0.0);
        }
 
        // calculate X position
        pair<int,int>* match = new pair<int,int> [macros_.size()];
        for(int i = 0; i < pos_seq_.size(); i++) {
            match[pos_seq_[i]].first = i;
            match[neg_seq_[i]].second = i;
        }
 
        float* length = new float [macros_.size()];
        for(int i = 0; i < macros_.size(); i++)
            length[i] = 0.0;
 
        for(int i = 0; i < pos_seq_.size(); i++) {
            int b = pos_seq_[i];
            int p = match[b].second;
            macros_[b].SpecifyX(length[p]);
            float t = macros_[b].GetX() + macros_[b].GetWidth();
            for(int j = p; j < neg_seq_.size(); j++)
                if(t > length[j])
                    length[j] = t;
                else
                    break;
        }
        
        width_ = length[macros_.size() - 1];
 
        // calulate Y position
        int* pos_seq = new int[pos_seq_.size()];
        int num_blocks = pos_seq_.size();
        for(int i = 0; i < num_blocks; i++)
            pos_seq[i] = pos_seq_[num_blocks - 1 - i];
 
        for(int i = 0; i < num_blocks; i++) {
            match[pos_seq[i]].first = i;
            match[neg_seq_[i]].second = i;
        }
 
        for(int i = 0; i < num_blocks; i++)
            length[i] = 0.0;
 
        for(int i = 0; i < num_blocks; i++) {
            int b = pos_seq[i];
            int p = match[b].second;
            macros_[b].SpecifyY(length[p]);
            float t = macros_[b].GetY() + macros_[b].GetHeight();
            for(int j = p; j < num_blocks; j++)
                if(t > length[j])
                    length[j] = t;
                else
                    break;
        }
 
        height_ = length[num_blocks - 1];
        area_ = width_ * height_;
    }

    
    void SimulatedAnnealingCore::SingleSwap(bool flag) {
        int index1 = (int)(floor((distribution_)(generator_) * macros_.size()));
        int index2 = (int)(floor((distribution_)(generator_) * macros_.size()));
        while(index1 == index2) {
            index2 = (int)(floor((distribution_)(generator_) * macros_.size()));
        }
        
        if(flag)
            swap(pos_seq_[index1], pos_seq_[index2]);
        else
            swap(neg_seq_[index1], neg_seq_[index2]);
    }

    
    void SimulatedAnnealingCore::DoubleSwap() {
        int index1 = (int)(floor((distribution_)(generator_) * macros_.size()));
        int index2 = (int)(floor((distribution_)(generator_) * macros_.size()));
        while(index1 == index2) {
            index2 = (int)(floor((distribution_)(generator_) * macros_.size()));
        }
        
        swap(pos_seq_[index1], pos_seq_[index2]);
        swap(neg_seq_[index1], neg_seq_[index2]);
    }

    void SimulatedAnnealingCore::Flip() {
        for(int i = 0; i < macros_.size(); i++) 
            macros_[i].Flip(flip_flag_);
    }

    void SimulatedAnnealingCore::SingleFlip() {
        CalculateWirelength();
        float wirelength_best = wirelength_;
        int action_id = 0;
        flip_flag_ = true;
        macros_[0].Flip(flip_flag_);
        CalculateWirelength();
        if(wirelength_best > wirelength_) {
            wirelength_ = wirelength_best;
            action_id = 1;
        }

        flip_flag_ = false;
        macros_[0].Flip(flip_flag_);
        CalculateWirelength();
        if(wirelength_best > wirelength_) {
            wirelength_ = wirelength_best;
            action_id = 2;
        }

        flip_flag_ = true;
        macros_[0].Flip(flip_flag_);
        CalculateWirelength();
        if(wirelength_best > wirelength_) {
            wirelength_ = wirelength_best;
            action_id = 3;
        }
        
        if(action_id == 3) {
            return;    
        } else {
            flip_flag_ = false;
            macros_[0].Flip(flip_flag_);
            if(action_id == 0) {
                CalculateWirelength();
                return;
            } else {
                flip_flag_ = true;
                macros_[0].Flip(flip_flag_);
                if(action_id == 1) {
                    CalculateWirelength();
                    return;
                } else {
                    flip_flag_ = false;
                    macros_[0].Flip(flip_flag_);
                    CalculateWirelength();
                    return;
                }
            }
        }
    }

    
    void SimulatedAnnealingCore::Perturb() {
        if(macros_.size() == 1) {
            CalculateWirelength();
            //cout << "Before Flip:  " << wirelength_ << "  ";
            Flip();
            //CalculateWirelength();
            //cout << "After Flip:  " << wirelength_ << endl;
            return;
        }

        pre_pos_seq_ = pos_seq_;
        pre_neg_seq_ = neg_seq_;
        pre_width_ = width_;
        pre_height_ = height_;
        pre_area_ = area_;
        pre_wirelength_ = wirelength_;
        pre_outline_penalty_ = outline_penalty_;

        float op = (distribution_)(generator_);
        if(op <= flip_prob_) {
            action_id_ = 1;
            float prob = (distribution_)(generator_);
            if(prob <= 0.5)
                flip_flag_ = true;
            else
                flip_flag_ = false;
           
            Flip();
        } else if (op <= pos_swap_prob_) {
            action_id_ = 2;
            SingleSwap(true);
        } else if(op <= neg_swap_prob_) {
            action_id_ = 3;
            SingleSwap(false);
        } else {
            action_id_ = 4;
            DoubleSwap();
        }
        
        PackFloorplan();
    }

    
    void SimulatedAnnealingCore::Restore() {
        if(action_id_ == 1) {
            Flip();
        } else {
            pos_seq_ = pre_pos_seq_;
            neg_seq_ = pre_neg_seq_;
        }

        width_ = pre_width_;
        height_ = pre_height_;
        area_ = pre_area_;
        wirelength_ = pre_wirelength_;
        outline_penalty_ = pre_outline_penalty_;
    }
    
    
    void SimulatedAnnealingCore::CalculateOutlinePenalty() {
        outline_penalty_ = 0.0;
         
        if(width_ > outline_width_ && height_ > outline_height_)
            outline_penalty_ = width_ * height_ - outline_width_ * outline_height_;
        else if(width_ > outline_width_ && height_ <= outline_height_)
            outline_penalty_ = (width_ - outline_width_) * outline_height_;
        else if(width_ <= outline_width_ && height_ > outline_height_)
            outline_penalty_ = outline_width_ * (height_ - outline_height_);
        else 
            outline_penalty_ = 0.0;
    }

    
    void SimulatedAnnealingCore::CalculateWirelength() {
        wirelength_ = 0.0;
        std::vector<Net*>::iterator net_iter = nets_.begin();
        for(net_iter; net_iter != nets_.end(); net_iter++) {
            vector<string> blocks = (*net_iter)->blocks_;
            vector<string> terminals = (*net_iter)->terminals_;
            
            if(blocks.size() == 0)
                continue;
            
            int weight = (*net_iter)->weight_;
            float lx = FLT_MAX;
            float ly = FLT_MAX;
            float ux = 0.0;
            float uy = 0.0;
 
            for(int i = 0; i < blocks.size(); i++) {
                float x = macros_[macro_map_[blocks[i]]].GetX() + macros_[macro_map_[blocks[i]]].GetPinX();
                float y = macros_[macro_map_[blocks[i]]].GetY() + macros_[macro_map_[blocks[i]]].GetPinY();
                lx = min(lx, x);
                ly = min(ly, y);
                ux = max(ux, x);
                uy = max(uy, y);
            }
 
            for(int i = 0; i < terminals.size(); i++) {
                float x = terminal_position_[terminals[i]].first;
                float y = terminal_position_[terminals[i]].second;
                lx = min(lx, x);
                ly = min(ly, y);
                ux = max(ux, x);
                uy = max(uy, y);
            }
 
            wirelength_ += (abs(ux - lx) + abs(uy - ly)) * weight;
        }
    }

    float SimulatedAnnealingCore::NormCost(float area, float wirelength, float outline_penalty) {
        float cost = alpha_ * area / norm_area_;
        if(norm_wirelength_ > 0.0)
            cost += beta_ * wirelength_ / norm_wirelength_;

        if(norm_outline_penalty_ > 0.0)
            cost += gamma_ * outline_penalty_ / norm_outline_penalty_;

        return cost;
    }

    void SimulatedAnnealingCore::Initialize() {
        vector<float> area_list;
        vector<float> wirelength_list;
        vector<float> outline_penalty_list;

        norm_area_ = 0.0;
        norm_wirelength_ = 0.0;
        norm_outline_penalty_ = 0.0;

        for(int i = 0; i < perturb_per_step_; i++) {
            Perturb();
            CalculateWirelength();
            CalculateOutlinePenalty();

            area_list.push_back(area_);
            wirelength_list.push_back(wirelength_);
            outline_penalty_list.push_back(outline_penalty_);
            
            norm_area_ += area_;
            norm_wirelength_ += wirelength_;
            norm_outline_penalty_ = outline_penalty_;
        }
        
        norm_area_ = norm_area_ / perturb_per_step_;
        norm_wirelength_ = norm_wirelength_ / perturb_per_step_;
        norm_outline_penalty_ = norm_outline_penalty_ / perturb_per_step_;

        vector<float> norm_cost_list;
        float norm_cost = 0.0;
        for(int i = 0; i < area_list.size(); i++) {
            norm_cost = NormCost(area_list[i], wirelength_list[i], outline_penalty_list[i]);
            norm_cost_list.push_back(norm_cost);
        }

        float delta_cost = 0.0;
        for(int i = 1; i < norm_cost_list.size(); i++)
            delta_cost += abs(norm_cost_list[i] - norm_cost_list[i-1]);

        delta_cost = delta_cost / (norm_cost_list.size() - 1);
        init_T_ = (-1) * delta_cost / log(init_prob_);
        cout << "Finish Initialization" << endl;
    }
    
    void SimulatedAnnealingCore::FastSA()
    {
        int step = 1;
        
        float pre_cost = NormCost(area_, wirelength_, outline_penalty_);
        float cost = pre_cost;
        float delta_cost = 0.0;
        vector<Macro> best_macros;
        float best_cost = cost;
        vector<int> best_pos_seq = pos_seq_;
        vector<int> best_neg_seq = neg_seq_;
        cout << "Enter FastSA stage" << endl;

        float rej_num = 0.0;
        float T = init_T_;
        float rej_threshold = rej_ratio_ * perturb_per_step_;
    

        while(step <= max_num_step_ && rej_num <= rej_threshold) {
            rej_num = 0.0;
            for(int i = 0; i < perturb_per_step_; i++) {
                Perturb();
                CalculateWirelength();
                //cout << "wirelength_   " << wirelength_ << "   ";
                CalculateOutlinePenalty();
                //cout << "outline_penalty_   " << outline_penalty_ << "   ";
                //cout << endl;
                cost = NormCost(area_, wirelength_, outline_penalty_);
        
                delta_cost = cost - pre_cost;
                float num = distribution_(generator_);
                float prob = (delta_cost > 0.0) ? exp((-1) * delta_cost / T) : 1;
                
                if(delta_cost <= 0 || num <= prob) {
                    pre_cost = cost;
                    if(cost < best_cost) {
                        best_cost = cost;
                        best_pos_seq = pos_seq_;
                        best_neg_seq = neg_seq_;
                        best_macros = macros_;
                    }
                } else {
                    rej_num += 1.0;
                    Restore();
                }
            }

            step++;
            
            //if(step <= k_)
            //    T = init_T_ / (step * c_);
            //else
            //    T = init_T_ / step;
            T = T * 0.99;
        }
       
        
        macros_ = best_macros;
        pos_seq_ = best_pos_seq;
        neg_seq_ = best_neg_seq;

        PackFloorplan();
    }
    

    void Run(SimulatedAnnealingCore* sa) { sa->Run(); }

    void ParseMacroFile(vector<Macro>& macros, float halo_width,  string file_name) {
        cout << "Enter ParseMacroFile" << endl;
        unordered_map<string, pair<float, float> > pin_loc;
        fstream f;
        string line;
        vector<string> content;
        f.open(file_name, ios::in);
        while(getline(f, line))
            content.push_back(line);

        f.close();
        for(int i = 0; i < content.size(); i++) {
            vector<string> words = Split(content[i]);
            string name = words[0];
            float pin_x = stof(words[3]) + halo_width;
            float pin_y = stof(words[4]) + halo_width;
            pin_loc[name] = pair<float, float>(pin_x, pin_y);
        }

        for(int i = 0; i < macros.size(); i++) {
            float pin_x = pin_loc[macros[i].GetName()].first;
            float pin_y = pin_loc[macros[i].GetName()].second;
            macros[i].SpecifyPinPosition(pin_x, pin_y);
        }

        cout << "Finish ParseMacroFile" << endl;
    }
    


    // Pin Alignment Engine
    void PinAlignment(vector<Cluster*>& clusters, float halo_width,  int num_thread, int num_run, unsigned seed) {
        cout << "Begin Pin Alignment Engine" << endl;
    
        // parameterse related to fastSA
        float init_prob = 0.95;
        float rej_ratio = 0.99;
        int max_num_step = 5000;
        int k = 5;
        float c = 100.0;
        float alpha = 0.3;
        float beta = 0.4;
        float gamma = 0.3;
        float flip_prob = 0.2;
        float pos_swap_prob = 0.3;
        float neg_swap_prob = 0.3;
        float double_swap_prob = 0.2;
                
        
        for(int i = 0; i < clusters.size(); i++) {
            if(clusters[i]->GetNumMacro() > 0) {
                string name = clusters[i]->GetName();
                for(int j = 0; j < name.size(); j++) 
                    if(name[j] == '/')
                        name[j] = '*';
            

                cout << "macro_cluster:   " << name << endl;

                float lx = clusters[i]->GetX();
                float ly = clusters[i]->GetY();
                float ux = lx + clusters[i]->GetWidth();
                float uy = ly + clusters[i]->GetHeight();
                float outline_width = ux - lx;
                float outline_height = uy - ly;

                // deal with pin position
                unordered_map<string, pair<float, float> > terminal_position;
                for(int j = 0; j < clusters.size(); j++) {
                    if(j != i) {
                        string terminal_name = clusters[j]->GetName();
                        float terminal_x = clusters[j]->GetX() + clusters[j]->GetWidth() / 2.0;
                        float terminal_y = clusters[j]->GetY() + clusters[j]->GetHeight() / 2.0;
                        terminal_x -= lx;
                        terminal_y -= ly;
                        terminal_position[terminal_name] = pair<float, float>(terminal_x, terminal_y);
                    }
                }


                // deal with macros
                vector<Macro> macros = clusters[i]->GetMacros();
                string macro_file = string("./rtl_mp/") + name + string(".txt.block");
                ParseMacroFile(macros, halo_width,  macro_file);
                
                // deal with nets
                vector<Net*> nets;
                string net_file = string("./rtl_mp/") + name + string(".txt.net");
                block_placement::ParseNetFile(nets, terminal_position, net_file.c_str());                
                
                int perturb_per_step = 50 * macros.size();

                std::mt19937 rand_generator(seed);
                vector<int> seed_list;
                for(int j = 0; j < num_thread; j++)
                    seed_list.push_back((unsigned)rand_generator());

                int remaining_run = num_run;
                int run_thread = num_thread;
                int sa_id = 0;
        
                cout << "Begin SA"  << endl;
                vector<SimulatedAnnealingCore*> sa_vector;
                while(remaining_run > 0) {
                    run_thread = num_thread;
                    if(remaining_run < num_thread)
                        run_thread = remaining_run;

                    for(int j = 0; j < run_thread; j++) {
                        SimulatedAnnealingCore* sa = new SimulatedAnnealingCore(macros, nets, terminal_position,
                                                         outline_width, outline_height, 
                                                         init_prob, rej_ratio, max_num_step, 
                                                         k, c, perturb_per_step,
                                                         alpha, beta, gamma, 
                                                         flip_prob, pos_swap_prob,  neg_swap_prob, double_swap_prob, 
                                                         seed_list[j]);
                    
                        sa_vector.push_back(sa);
                    }
            
                    vector<thread> threads;
                    for(int j = 0; j < run_thread; j++)
                        threads.push_back(thread(Run, sa_vector[sa_id++]));
                   
                    cout << "outline_width:   " << outline_width << "    ";
                    cout << "outline_height:  " << outline_height << "   ";
                    cout << endl;

                    cout << "begin running SA" << endl;
                    for(auto &th : threads)
                        th.join();

                    cout << "Finish running SA" << endl;
                    remaining_run = remaining_run - run_thread;
                }

                cout << "Finish SA" << endl;
                int min_id = -1;
                float wirelength = FLT_MAX;
                
                for(int j = 0; j < sa_vector.size(); j++) {
                    string file_name = string("./rtl_mp/") + name + string("_") + to_string(j) + string("_final.txt");
                    sa_vector[j]->WriteFloorplan(file_name);
                }       
                
                
                for(int j = 0; j < sa_vector.size(); j++) 
                    if(sa_vector[j]->IsFeasible()) 
                        if(wirelength > sa_vector[j]->GetWirelength()) 
                            min_id = j;

                if(min_id == -1) {
                    throw std::invalid_argument(std::string("Invalid Floorplan.  Please increase the num_run!!!"));
                } else {
                    clusters[i]->SpecifyMacros(sa_vector[min_id]->GetMacros());
                }
                
                for(int j = 0; j < sa_vector.size(); j++) 
                    delete sa_vector[j];
            }
        }
        cout << "Finish Pin Alignment" << endl;
    }


}






