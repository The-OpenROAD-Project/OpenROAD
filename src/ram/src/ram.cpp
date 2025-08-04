/////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Precision Innovations Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "ram/ram.h"

#include "db_sta/dbNetwork.hh"
#include "layout.h"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "utl/Logger.h"

namespace ram {

using odb::dbBlock;
using odb::dbBTerm;
using odb::dbInst;
using odb::dbMaster;
using odb::dbNet;

using utl::RAM;

using std::vector;
using std::array;

////////////////////////////////////////////////////////////////

RamGen::RamGen() : db_(nullptr), logger_(nullptr) {
}

void RamGen::init(odb::dbDatabase* db, sta::dbNetwork* network, Logger* logger) {
    db_ = db;
    network_ = network;
    logger_ = logger;
}

dbInst* RamGen::makeInst(
    Layout* layout,
    const std::string& prefix,
    const std::string& name,
    dbMaster* master,
    const vector<std::pair<std::string, dbNet*>>& connections) {
    const auto inst_name = fmt::format("{}.{}", prefix, name);
    auto inst = dbInst::create(block_, master, inst_name.c_str());
    for (auto& [mterm_name, net] : connections) {
        auto mterm = master->findMTerm(mterm_name.c_str());
        if (!mterm) {
            logger_->error(
                RAM, 9, "term {} of cell {} not found.", name, master->getName());
        }
        auto iterm = inst->getITerm(mterm);
        iterm->connect(net);
    }


    layout->addElement(std::make_unique<Element>(inst));
    return inst;
}

dbInst* RamGen::makeCellInst(
    Cell* cell,
    const std::string& prefix,
    const std::string& name,
    dbMaster* master,
    const vector<std::pair<std::string, dbNet*>>& connections) {
    const auto inst_name = fmt::format("{}.{}", prefix, name);
    auto inst = dbInst::create(block_, master, inst_name.c_str());
    for (auto& [mterm_name, net] : connections) {
        auto mterm = master->findMTerm(mterm_name.c_str());
        if (!mterm) {
            logger_->error(
               RAM, 11, "term {} of cell {} not found.", name, master->getName());
        }
        auto iterm = inst->getITerm(mterm);
        iterm->connect(net);
    }

   cell->addInst(inst);
    return inst;
}


dbNet* RamGen::makeNet(const std::string& prefix, const std::string& name) {
    const auto net_name = fmt::format("{}.{}", prefix, name);
    return dbNet::create(block_, net_name.c_str());
}

dbNet* RamGen::makeBTerm(const std::string& name) {
    auto net = dbNet::create(block_, name.c_str());
    dbBTerm::create(net, name.c_str());
    return net;
}

dbNet* RamGen::makeOutputBTerm(const std::string& name) {
    auto net = dbNet::create(block_, name.c_str());
    auto bTerm = dbBTerm::create(net, name.c_str());

    bTerm->setIoType(odb::dbIoType::OUTPUT);
    return net;
}

std::unique_ptr<Element> RamGen::make_bit(const std::string& prefix,
        const int read_ports,
        dbNet* clock,
        vector<odb::dbNet*>& select,
        dbNet* data_input,
        vector<odb::dbNet*>& data_output) {
    auto layout = std::make_unique<Layout>(odb::horizontal);

    auto storage_net = makeNet(prefix, "storage");

    // Make Storage latch
    makeInst(layout.get(),
             prefix,
             "bit",
             storage_cell_,
    {{"GATE", clock}, {"D", data_input}, {"Q", storage_net}});

    // Make ouput tristate driver(s) for read port(s)
    for (int read_port = 0; read_port < read_ports; ++read_port) {
        makeInst(layout.get(),
                 prefix,
                 fmt::format("obuf{}", read_port),
        tristate_cell_, {
            {"A", storage_net},
            {"TE_B", select[read_port]},
            {"Z", data_output[read_port]}
        });
    }

    return std::make_unique<Element>(std::move(layout));
}

std::unique_ptr<Element> RamGen::make_byte(
    const std::string& prefix,
    const int read_ports,
    dbNet* clock,
    dbNet* write_enable,
    const vector<dbNet*>& selects,
    const array<dbNet*,8>& data_input,
    const vector<array<dbNet*, 8>>& data_output) {
    auto layout = std::make_unique<Layout>(odb::horizontal);


    vector<dbNet*> select_b_nets(selects.size());
    for (int i = 0; i < selects.size(); ++i) {
        select_b_nets[i] = makeNet(prefix, fmt::format("select{}_b", i));
    }

    // vector<dbNet*> select_b_nets = decoder_selects(prefix, read_ports);

    auto gclock_net = makeNet(prefix, "gclock");
    auto we0_net = makeNet(prefix, "we0");

    for (int bit = 0; bit < 8; ++bit) {
        auto name = fmt::format("{}.bit{}", prefix, bit);
        vector<dbNet*> outs;
        for (int read_port = 0; read_port < read_ports; ++read_port) {
            outs.push_back(data_output[read_port][bit]);
        }
        layout->addElement(make_bit(
                               name, read_ports, gclock_net, select_b_nets, data_input[bit], outs));
    }

    // Make clock gate
    makeInst(layout.get(),
             prefix,
             "cg",
             clock_gate_cell_,
    {{"CLK", clock}, {"GATE", we0_net}, {"GCLK", gclock_net}});

    // Make clock and
    // this AND gate needs to be fed a net created by a decoder
    // adding any net will automatically connect with any port
    makeInst(layout.get(),
             prefix,
             "gcand",
             and2_cell_,
    {{"A", selects[0]}, {"B", write_enable}, {"X", we0_net}});

    // Make select inverters
    for (int i = 0; i < selects.size(); ++i) {
        makeInst(layout.get(),
                 prefix,
                 fmt::format("select_inv_{}", i),
                 inv_cell_,
        {{"A", selects[i]}, {"Y", select_b_nets[i]}});
    }

    // Make clock inverter
   // makeInst(layout.get(),
   //          prefix,
   //          "clock_inv",
   //          inv_cell_,
   // {{"A", clock}, {"Y", clock_b_net}});



    return std::make_unique<Element>(std::move(layout));
}

std::unique_ptr<Element> RamGen::create_and_layer (const std::string& prefix,
        const int word_count, const int read_ports,
        const std::vector<odb::dbNet*>& selects, const std::vector<odb::dbNet*>& addr_nets) {

    auto layout = std::make_unique<Layout>(odb::horizontal);

    //can make this an AND gate layer method
    //places appropriate number of AND gates for each word

    //calculates number of and gate layers needed
    int layers = std::log2(word_count) - 1;

    dbNet* prev_net = nullptr; //net to store previous and gate output
    for (int i = 0; i < layers; ++i) {
        auto input_net = makeNet(prefix, fmt::format("layer_in{}", i));
        //sets up first AND gate, closest to byte's select + write enable gate
	if (i == 0 && i == layers - 1) {
	    makeInst (layout.get(), prefix, fmt::format("and_layer{}",i), and2_cell_,
		{{"A", addr_nets[i]}, {"B", addr_nets[i + 1]}, {"X", selects[0]}});
	   prev_net = input_net;
	} else if (i == 0) {
            makeInst(layout.get(), prefix, fmt::format("and_layer{}", i), and2_cell_,
            {{"A", addr_nets[i]}, {"B", input_net}, {"X", selects[0]}});
            prev_net = input_net;
        } else if (i == layers - 1) { //last AND gate layer
            makeInst(layout.get(), prefix, fmt::format("and_layer{}", i), and2_cell_, {
                {"A", addr_nets[i]}, {"B", addr_nets[i + 1]}, {"X", prev_net}
            });
            prev_net = input_net;
        } else { //middle AND gate layers
            makeInst(layout.get(), prefix, fmt::format("and_layer{}", i), and2_cell_,
            {{"A", addr_nets[i]}, {"B", input_net}, {"X", prev_net}});
            prev_net = input_net;
        }


    }


    vector<vector<dbNet*>> and_layer_inputs(word_count);


    return std::make_unique<Element>(std::move(layout));
}


std::vector<dbNet*> RamGen::decoder_selects(const std::string& prefix, const int read_ports) {

    std::vector<dbNet*> decoder_select_nets(read_ports);
    for (int i = 0 ; i < read_ports; ++i) {
        decoder_select_nets[i] = makeNet(prefix, fmt::format("decoder{}", i));
    }
    return decoder_select_nets;
}


dbMaster* RamGen::findMaster(
    const std::function<bool(sta::LibertyPort*)>& match,
    const char* name) {
    dbMaster* best = nullptr;
    float best_area = std::numeric_limits<float>::max();

    for (auto lib : db_->getLibs()) {
        for (auto master : lib->getMasters()) {
            auto cell = network_->dbToSta(master);
            if (!cell) {
                continue;
            }
            auto liberty = network_->libertyCell(cell);
            if (!liberty) {
                continue;
            }

	    if (!match) {
	       if (liberty->portCount() == 0) {
	            best_area = liberty->area();
		    best = master;
	       }
	       continue;
	    }

            auto port_iter = liberty->portIterator();

            sta::ConcretePort* out = nullptr;
            while (port_iter->hasNext()) {
                auto lib_port = port_iter->next();
                auto dir = lib_port->direction();
                if (dir->isAnyOutput()) {
                    if (!out) {
                        out = lib_port;
                    } else {
                        out = nullptr;  // no multi-output gates
                        break;
                    }
                }
            }

            delete port_iter;
            if (!out || !match(out->libertyPort())) {
                continue;
            }

            if (liberty->area() < best_area) {
                best_area = liberty->area();
                best = master;
            }
        }
    }

    if (!best) {
        logger_->error(RAM, 10, "Can't find {} cell", name);
    }
    logger_->info(RAM, 16, "Selected {} cell {}", name, best->getName());
    return best;
}

void RamGen::findMasters() {
    if (!inv_cell_) {
        inv_cell_ = findMaster(
        [this](sta::LibertyPort* port) {
            return port->libertyCell()->isInverter();
        },
        "inverter");
    }

    if (!tristate_cell_) {
        tristate_cell_ = findMaster(
        [this](sta::LibertyPort* port) {
            if (!port->direction()->isTristate()) {
                return false;
            }
            auto function = port->function();
            return function->op() != sta::FuncExpr::op_not;
        },
        "tristate");
    }

    if (!and2_cell_) {
        and2_cell_ = findMaster(
        [this](sta::LibertyPort* port) {
            if (!port->direction()->isOutput()) {
                return false;
            }
            auto function = port->function();
            return function && function->op() == sta::FuncExpr::op_and
                   && function->left()->op() == sta::FuncExpr::op_port
                   && function->right()->op() == sta::FuncExpr::op_port;
        },
        "and2");
    }

    if (!storage_cell_) {
        // FIXME
        storage_cell_ = findMaster(
        [this](sta::LibertyPort* port) {
            if (!port->direction()->isOutput()) {
                return false;
            }
            auto function = port->function();
            return function && function->op() == sta::FuncExpr::op_and
                   && function->left()->op() == sta::FuncExpr::op_port
                   && function->right()->op() == sta::FuncExpr::op_port;
        },
        "storage");
    }

    if (!clock_gate_cell_) {
        clock_gate_cell_ = findMaster(
        [this](sta::LibertyPort* port) {
            return port->libertyCell()->isClockGate();
        },
        "clock gate");
    }
    //for input buffers
    if (!buffer_cell_) {
        buffer_cell_ = findMaster(
        [this](sta::LibertyPort* port) {
            return port->libertyCell()->isBuffer();
        },
        "buffer");
    }
}

void RamGen::generate(const int bytes_per_word,
                      const int word_count,
                      const int read_ports,
                      dbMaster* storage_cell,
                      dbMaster* tristate_cell,
                      dbMaster* inv_cell) {
    const int bits_per_word = bytes_per_word * 8;
    const std::string ram_name
        = fmt::format("RAM{}x{}", word_count, bits_per_word);

    logger_->info(RAM, 3, "Generating {}", ram_name);

    storage_cell_ = storage_cell;
    tristate_cell_ = tristate_cell;
    inv_cell_ = inv_cell;
    and2_cell_ = nullptr;
    clock_gate_cell_ = nullptr;
    buffer_cell_ = nullptr; 
    findMasters();

    auto chip = db_->getChip();
    if (!chip) {
        chip = odb::dbChip::create(db_);
    }

    block_ = chip->getBlock();
    if (!block_) {
        block_ = odb::dbBlock::create(chip, ram_name.c_str());
    }

    Layout layout(odb::horizontal);

    auto clock = makeBTerm("clk");

    vector<dbNet*> write_enable(bytes_per_word, nullptr);
    for (int byte = 0; byte < bytes_per_word; ++byte) {
        auto in_name = fmt::format("we[{}]", byte);
        write_enable[byte] = makeBTerm(in_name);
    }

    //input bterms
    int numImputs = std::log2(word_count);
    vector<dbNet*> addr(numImputs, nullptr);
    for (int i = 0; i < numImputs; ++i) {
        addr[i] = (makeBTerm(fmt::format("addr[{}]", i)));
    }

    //vector of nets storing inverter nets
    vector<dbNet*> inv_addr(numImputs);
    for (int i = 0; i < numImputs; ++i) {
        inv_addr[i] = makeNet("inv", fmt::format("addr[{}]", i));
    }


    //decoder_layer nets 
    vector<vector<dbNet*>> and_layer_nets(word_count, vector<dbNet*> (numImputs));
    for (int word = 0; word < word_count; ++word) {
        int word_num = word;
        for (int input = 0; input < numImputs; ++input) { //start at right most bit
            if (word_num % 2 == 0) {
                //places inverter for each input
                and_layer_nets[word][input] = inv_addr[input];
            } else { //puts original input in invert nets
                and_layer_nets[word][input] = addr[input];
            }
            word_num /= 2;
        }
    }
    Layout input_buffer_layer(odb::horizontal); //input buffer layer

    auto inv_layer = std::make_unique<Layout>(odb::vertical); //inverter column
    vector<dbNet*> word_decoder_nets;

    for (int col = 0; col < bytes_per_word; ++col) {
        array<dbNet*, 8> D; //array for b-term for external inputs
        array<dbNet*, 8> D_nets; //net for buffers
        for (int bit = 0; bit < 8; ++bit) {
            D[bit] = makeBTerm(fmt::format("D[{}]", bit + col * 8));
            D_nets[bit] = makeNet(fmt::format("D_nets[{}]", bit + col * 8),"net");
        }

        vector<array<dbNet*, 8>> Q;
        //if readports == 1, only have Q outputs
        if (read_ports == 1) {
            array<dbNet*, 8> d;
            for (int bit = 0; bit < 8; ++bit) {
                auto out_name = fmt::format("Q[{}]", bit + col * 8);
                d[bit] = makeOutputBTerm(out_name);
            }
            Q.push_back(d);
        } else {
            for (int read_port = 0; read_port < read_ports; ++read_port) {
                array<dbNet*, 8> d;
                // add in readport = 1, then name outputs as just Q
                for (int bit = 0; bit < 8; ++bit) {
                    auto out_name = fmt::format("Q{}[{}]", read_port, bit + col * 8);
                    d[bit] = makeOutputBTerm(out_name);
                }
                Q.push_back(d);
            }
	} 

        vector<vector<dbNet*>> decoder_select_nets (word_count);

        auto column = std::make_unique<Layout>(odb::vertical); //byte column
        auto and_layer = std::make_unique<Layout>(odb::vertical); //inverter column

        //auto test_layout = std::make_unique<Layout>(odb::horizontal);
        for (int row = 0; row < word_count; ++row) {


            auto name = fmt::format("storage_{}_{}", row, col);

	    if (word_count == 2) {
		word_decoder_nets.clear();
	    	word_decoder_nets.push_back(row == 0 ? inv_addr[0] : addr[0]);
	    } else {
                word_decoder_nets = decoder_selects(name, read_ports);
	    
	    }
            //creates nets that decoders will use
            decoder_select_nets.push_back(word_decoder_nets);


            //makes bytes
            column->addElement(make_byte(
                                   name,
                                   read_ports,
                                   clock,
                                   write_enable[col],
                                   word_decoder_nets,
                                   D_nets,
                                   Q));

            //adds elements to new column
            and_layer->addElement(create_and_layer(fmt::format("decoder{}", row),
                                                   word_count, read_ports, word_decoder_nets, and_layer_nets[row]));



        }

        for (int bit = 0; bit < 8; ++bit) {
           makeInst(&input_buffer_layer,
                     "buffer",
                     fmt::format("in[{}]", bit),
                     buffer_cell_,
            { {"A", D[bit]}, {"X", D_nets[bit]} });

	}

//	input_buffer_layer.position(odb::Point(0,0), 7900); //tester for offset for buffer
	//adds input buffer layer to column
        //column->addElement(std::make_unique<Element>(std::move(input_buffer_layer)));

        //places byte first
        layout.addElement(std::make_unique<Element>(std::move(column)));
        //places inverters
        layout.addElement(std::make_unique<Element>(std::move(and_layer)));

    }

    //check for AND gate, specific case for 2 words
    if (numImputs > 1) {
        for (int i = 0; i < numImputs; ++i) {
            makeInst(inv_layer.get(),
                     "decoder",
                     fmt::format("inv_{}", i),
                     inv_cell_,
            {{"A", addr[i]}, {"Y", inv_addr[i]}});
        }
    } else  {
        makeInst (inv_layer.get(),
                  "decoder",
                  fmt::format("inv_{}", 0),
                  inv_cell_,
        {{"A", addr[0]}, {"Y", inv_addr[0]}});
    }

    //adds column of inverters
    layout.addElement(std::make_unique<Element>(std::move(inv_layer)));

    auto cellBTerm  = makeBTerm ("cell_in");
    auto cellNet = makeNet ("net", "cell_out");
   
    vector<std::unique_ptr<CellLayout>> cell_layouts;

    Grid new_grid (odb::horizontal);

    for (int num_layouts = 0; num_layouts < 4; ++num_layouts) {
        auto new_layout = std::make_unique<CellLayout>(odb::vertical);
	for (int num_cells = 0; num_cells < 4; ++num_cells) {
	   auto new_cell = std::make_unique<Cell>();
	   for (int num_inst = 0; num_inst < 4; ++num_inst) {
	      makeCellInst(new_cell.get(), fmt::format("layout_{}", num_layouts),
			   fmt::format("cell_{}inst{}", num_cells, num_inst), inv_cell_, 
			   {{"A", cellBTerm}, {"Y", cellNet}});
	   }
        new_layout->addCell(std::move (new_cell));
	}
    new_grid.addLayout(std::move(new_layout));       
    } 

    odb::Point layout_orig (0,0);
    new_grid.setOrigin(layout_orig);
    new_grid.gridInit();
    new_grid.placeGrid();
    
    


    //layout.position(odb::Point(0, 0));
}

}  // namespace ram
