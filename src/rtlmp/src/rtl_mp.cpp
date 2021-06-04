#include "rtlmp/rtl_mp.h"

#include<iostream>
#include<string>
#include<vector>
#include<unordered_map>
#include<random>
#include<algorithm>
#include<fstream>
#include<map>

#include "rtlmp/pin_alignment.h"
#include "rtlmp/shape_engine.h"
#include "rtlmp/block_placement.h"
#include "rtlmp/util.h"

namespace shape_engine {
    std::map<Orientation, Orientation> Macro::FLIP_X_TABLE {
        {R0, MX},
        {MX, R0},
        {R90, MY90},
        {MY90, R90},
        {R180, MY},
        {MY, R180},
        {R270, MX90},
        {MX90, R270}
    };
 
    std::map<Orientation, Orientation> Macro::FLIP_Y_TABLE {
        {R0, MY},
        {MY, R0},
        {R180, MX},
        {MX, R180},
        {R90, MX90},
        {MX90, R90},
        {R270, MY90},
        {MY90, R270}
    };


}




namespace ord {
    using std::vector;
    using shape_engine::Cluster;
    using shape_engine::Macro;
    using block_placement::Block;
    using std::string;
    using std::unordered_map;
    using std::ofstream;
    using std::to_string;
    using std::cout;
    using std::endl;
    using std::stoi;
    using std::stof;


    void RTLMP(const char* config_file) {
        const char* block_file = "./rtl_mp/partition.txt.block";
        const char* net_file = "./rtl_mp/partition.txt.net";
        
        // parameters defined in config_file
        float min_aspect_ratio = 0.33;
        float dead_space = 0.1;
        float halo_width = 2.0;
        
        string region_file = string("macro_blockage.txt");
        
        int num_thread = 5;
        int num_run = 5;
        float heat_rate = 0.5;
        
        unsigned seed = 0;
        
        int num_level = 5;
        int num_worker = 10;
        
        float alpha = 0.4;
        float beta = 0.3;
        float gamma = 0.3;
        float boundary_weight = 0.06;
        float macro_blockage_weight = 0.08;

        float resize_prob = 0.4;
        float pos_swap_prob = 0.2;
        float neg_swap_prob = 0.2;
        float double_swap_prob = 0.2;

        float init_prob = 0.95;
        float rej_ratio = 0.95;
        int k = 50;
        float c = 100;
        int max_num_step = 300;
        int perturb_per_step = 3000;
        
        unordered_map<string, string> params = ParseConfigFile(config_file);
        
        unordered_map<string, string>::iterator param_iter;
        string param;

        param = "min_aspect_ratio";
        param_iter = params.find(param);
        if(param_iter != params.end())
            min_aspect_ratio = stof(param_iter->second);
        cout << "min_aspect_ratio:  " << min_aspect_ratio << endl;
        

        param = "dead_space";
        param_iter = params.find(param);
        if(param_iter != params.end())
            dead_space = stof(param_iter->second);
        cout << "dead_space:  " << dead_space << endl;


        param = "halo_width";
        param_iter = params.find(param);
        if(param_iter != params.end())
            halo_width = stof(param_iter->second);
        cout << "halo_width:  " << halo_width << endl; 


        param = "region_file";
        param_iter = params.find(param);
        if(param_iter != params.end())
            region_file = param_iter->second;

        param = "num_thread";
        param_iter = params.find(param);
        if(param_iter != params.end())
            num_thread = stoi(param_iter->second);
        cout << "num_thread:  " << num_thread << endl;


        param = "num_run";
        param_iter = params.find(param);
        if(param_iter != params.end())
            num_run = stoi(param_iter->second);
        cout << "num_run:  " << num_run << endl;

        param = "heat_rate";
        param_iter = params.find(param);
        if(param_iter != params.end())
            heat_rate = stof(param_iter->second);
        cout <<  "heat_rate:  " << heat_rate << endl;

        param = "num_level";
        param_iter = params.find(param);
        if(param_iter != params.end())
            num_level = stoi(param_iter->second);
        cout << "num_level:  " << num_level << endl;

        param = "num_worker";
        param_iter = params.find(param);
        if(param_iter != params.end())
            num_worker = stoi(param_iter->second);
        cout << "num_worker:  " << num_worker << endl;

        param = "alpha";
        param_iter = params.find(param);
        if(param_iter != params.end())
            alpha = stof(param_iter->second);
        cout << "alpha:  " << alpha << endl;

        param = "beta";
        param_iter = params.find(param);
        if(param_iter != params.end())
            beta = stof(param_iter->second);
        cout << "beta:  " << beta << endl;

        param = "gamma";
        param_iter = params.find(param);
        if(param_iter != params.end())
            gamma = stof(param_iter->second);
        cout << "gamma:  " << gamma << endl;

        param = "boundary_weight";
        param_iter = params.find(param);
        if(param_iter != params.end())
            boundary_weight = stof(param_iter->second);
        cout << "boundary_weight:  " << boundary_weight << endl;

        param = "macro_blockage_weight";
        param_iter = params.find(param);
        if(param_iter != params.end())
            macro_blockage_weight = stof(param_iter->second);
        cout << "macro_blockage_weight:  " << macro_blockage_weight << endl;

        param = "resize_prob";
        param_iter = params.find(param);
        if(param_iter != params.end())
            resize_prob = stof(param_iter->second);
        cout << "resize_prob:  " << resize_prob << endl;

        param = "pos_swap_prob";
        param_iter = params.find(param);
        if(param_iter != params.end())
            pos_swap_prob = stof(param_iter->second);
        cout << "pos_swap_prob:  " << pos_swap_prob << endl;

        param = "neg_swap_prob";
        param_iter = params.find(param);
        if(param_iter != params.end())
            neg_swap_prob = stof(param_iter->second);
        cout << "neg_swap_prob:   " << neg_swap_prob << endl;

        param = "double_swap_prob";
        param_iter = params.find(param);
        if(param_iter != params.end())
            double_swap_prob = stof(param_iter->second);
        cout << "double_swap_prob:  " << double_swap_prob << endl;

        param = "init_prob";
        param_iter = params.find(param);
        if(param_iter != params.end())
            init_prob = stof(param_iter->second);
        cout << "init_prob:  " << init_prob << endl;

        param = "rej_ratio";
        param_iter = params.find(param);
        if(param_iter != params.end())
            rej_ratio = stof(param_iter->second);
        cout << "rej_ratio:  " << rej_ratio << endl;

        param = "k";
        param_iter = params.find(param);
        if(param_iter != params.end())
            k = stoi(param_iter->second);
        cout << "k:   " << k << endl;

        param = "c";
        param_iter = params.find(param);
        if(param_iter != params.end())
            c = stof(param_iter->second);
        cout << "c:  " << c << endl;

        param = "max_num_step";
        param_iter = params.find(param);
        if(param_iter != params.end())
            max_num_step = stoi(param_iter->second);
        cout << "max_num_step:  " << max_num_step << endl;
 
        param = "perturb_per_step";
        param_iter = params.find(param);
        if(param_iter != params.end())
            perturb_per_step = stoi(param_iter->second);
        cout << "perturb_per_step:  " << perturb_per_step << endl;

        param = "seed";
        param_iter = params.find(param);
        if(param_iter != params.end())
            seed = stoi(param_iter->second);
        cout << "seed:  " << seed << endl;


        float outline_width = 0.0;
        float outline_height = 0.0;
        float outline_lx = 0.0;
        float outline_ly = 0.0;


        vector<Cluster*> clusters = shape_engine::ShapeEngine(outline_width, outline_height, outline_lx, outline_ly,
                min_aspect_ratio, dead_space, halo_width, block_file, num_thread, num_run, seed);


        vector<Block> blocks =  block_placement::Floorplan(clusters, outline_width, outline_height, 
                             net_file, region_file.c_str(), 
                             num_level, num_worker, heat_rate,  alpha, beta,  gamma, boundary_weight, macro_blockage_weight,
                             resize_prob, pos_swap_prob, neg_swap_prob, double_swap_prob,
                             init_prob, rej_ratio, max_num_step, k, c, perturb_per_step, seed);

        
        unordered_map<string, int> block_map;

        for(int i = 0; i < clusters.size(); i++) 
            block_map[blocks[i].GetName()] = i;
        
        for(int i = 0; i < clusters.size(); i++) {
            float x = blocks[block_map[clusters[i]->GetName()]].GetX();
            float y = blocks[block_map[clusters[i]->GetName()]].GetY();
            float width = blocks[block_map[clusters[i]->GetName()]].GetWidth();
            float height = blocks[block_map[clusters[i]->GetName()]].GetHeight();
            clusters[i]->SpecifyPos(x, y);
            clusters[i]->SpecifyFootprint(width, height);
        }

        pin_alignment::PinAlignment(clusters, halo_width, num_thread, num_run, seed);

    
        cout << "outline_width:   " << outline_width << "    ";
        cout << "outline_height:  " << outline_height << "    ";
        cout << endl;
        cout << "outline_lx:  " << outline_lx << "   ";
        cout << "outline_ly:  " << outline_ly << "   ";
        cout << endl;

        const char* openroad_filename = "./rtl_mp/macro_placement.cfg";
        ofstream file;
        file.open(openroad_filename);
        for(int i = 0; i < clusters.size(); i++) {
            if(clusters[i]->GetNumMacro() > 0) {
                float cluster_lx = clusters[i]->GetX();
                float cluster_ly = clusters[i]->GetY();
                vector<Macro> macros = clusters[i]->GetMacros();
                for(int j = 0; j < macros.size(); j++) {
                    string line = macros[j].GetName();
                    float lx = outline_lx + cluster_lx + macros[j].GetX() + halo_width;
                    float ly = outline_ly + cluster_ly + macros[j].GetY() + halo_width;
                    float width = macros[j].GetWidth() -  2 * halo_width;
                    float height = macros[j].GetHeight() - 2 * halo_width;
                    string orientation  = macros[j].GetOrientation();
                    
                    if(orientation == string("MX"))
                        line += string("  MX  ") + to_string(lx) + string("   ") + to_string(ly + height);
                    else if(orientation == string("MY")) 
                        line += string("  MY  ") + to_string(lx + width) + string("   ")  + to_string(ly) ;
                    else if(orientation == string("R180"))
                        line += string("  R180  ") + to_string(lx + width) + string("   ") + to_string(ly + height);
                    else 
                        line += string("  R0 ") + to_string(lx) + string("   ") + to_string(ly);

                    
                    file << line << endl;
                }
            }
        }
        
        file.close();

        const char* invs_filename = "./rtl_mp/macro_placement.tcl";
        file.open(invs_filename);
        for(int i = 0; i < clusters.size(); i++) {
            if(clusters[i]->GetNumMacro() > 0) {
                float cluster_lx = clusters[i]->GetX();
                float cluster_ly = clusters[i]->GetY();
                vector<Macro> macros = clusters[i]->GetMacros();
                for(int j = 0; j < macros.size(); j++) {
                    string line = "setObjFPlanBox Instance ";
                    line += macros[j].GetName() + string("   ");
                    float lx = outline_lx + cluster_lx + macros[j].GetX() + halo_width;
                    float ly = outline_ly + cluster_ly + macros[j].GetY() + halo_width;
                    float width = macros[j].GetWidth() -  2 * halo_width;
                    float height = macros[j].GetHeight() - 2 * halo_width;
                    line += to_string(lx) + string("   ") + to_string(ly) + string("  ");
                    line += to_string(lx + width) + string("  ") + to_string(ly + height);
                    file << line << endl;

                    line = string("dbSet [dbGet top.insts.name -p ");
                    line += macros[j].GetName() + string(" ].orient ");
                    string orientation  = macros[j].GetOrientation();
                    line += orientation;
                    file << line << endl;
                }
            }
        }
        
        file.close();





        // just for quick verification
        const char* floorplan_filename = "./rtl_mp/final_floorplan.txt";
        file.open(floorplan_filename);
        file << "outline_width:  " << outline_width << endl;
        file << "outline_height:  " << outline_height << endl;
        for(int i = 0; i < clusters.size(); i++) {
            float cluster_lx = clusters[i]->GetX();
            float cluster_ly = clusters[i]->GetY();
            float cluster_ux = cluster_lx + clusters[i]->GetWidth();
            float cluster_uy = cluster_ly + clusters[i]->GetHeight();
            string cluster_name = clusters[i]->GetName();
            file << cluster_name << "   ";
            file << cluster_lx << "    ";
            file << cluster_ly << "    ";
            file << cluster_ux << "    ";
            file << cluster_uy << "    ";
            file << endl;            
        }
        file << endl;


        for(int i = 0; i < clusters.size(); i++) {
            if(clusters[i]->GetNumMacro() > 0) {
                float cluster_lx = clusters[i]->GetX();
                float cluster_ly = clusters[i]->GetY();
                vector<Macro> macros = clusters[i]->GetMacros();
                for(int j = 0; j < macros.size(); j++) {
                    string name = macros[j].GetName();
                    float lx = cluster_lx + macros[j].GetX() + halo_width;
                    float ly = cluster_ly + macros[j].GetY() + halo_width;
                    float width = macros[j].GetWidth() -  2 * halo_width;
                    float height = macros[j].GetHeight() - 2 * halo_width;
                    file << name << "    ";
                    file << lx << "   ";
                    file << ly << "   ";
                    file << lx + width << "   ";
                    file << ly + height << "   ";
                    file << endl;
                }
            }
        }
        
        file.close();









    }

}















