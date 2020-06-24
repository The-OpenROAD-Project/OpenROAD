// BSD 3-Clause License

// Copyright (c) 2019, SCALE Lab, Brown University
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.

// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.

// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.

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

#include "BufferFanoutTransform.hpp"
#include "OpenPhySyn/DatabaseHandler.hpp"
#include "OpenPhySyn/Psn.hpp"
#include "OpenPhySyn/PsnGlobal.hpp"
#include "OpenPhySyn/PsnLogger.hpp"
#include "StringUtils.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace psn
{

int
BufferFanoutTransform::buffer(Psn* psn_inst, int max_fanout,
                              std::string buffer_cell)
{

    DatabaseHandler& handler = *(psn_inst->handler());
    LibraryCell*     cell    = handler.libraryCell(buffer_cell.c_str());
    std::unordered_map<std::string, Instance*> name_to_buffer;
    std::unordered_map<std::string, Net*>      name_to_net;
    int                                        buff_index = 0;
    int                                        net_index  = 0;
    if (!cell)
    {
        PSN_LOG_ERROR("Buffer", buffer_cell, "not found!");
        return -1;
    }
    auto buffer_input_pins  = handler.libraryInputPins(cell);
    auto buffer_output_pins = handler.libraryOutputPins(cell);
    if (buffer_input_pins.size() != 1)
    {
        PSN_LOG_ERROR("Invalid buffer cell, number of input pins is",
                      buffer_input_pins.size());
        return -1;
    }
    if (buffer_output_pins.size() != 1)
    {
        PSN_LOG_ERROR("Invalid buffer cell, number of output pins is",
                      buffer_output_pins.size());
        return -1;
    }
    if (max_fanout < 2)
    {
        PSN_LOG_ERROR("Invalid max_fanout value", buffer_output_pins.size(),
                      "minimum is 2");
        return -1;
    }
    LibraryTerm*      cell_in_pin  = *(buffer_input_pins.begin());
    LibraryTerm*      cell_out_pin = *(buffer_output_pins.begin());
    auto              nets         = handler.nets();
    std::vector<Net*> high_fanout_nets;
    for (auto& net : nets)
    {
        if (!handler.isPrimary(net) &&
            handler.fanoutCount(net) > (unsigned int)max_fanout)
        {
            high_fanout_nets.push_back(net);
        }
    }
    PSN_LOG_INFO("High fanout nets [", high_fanout_nets.size(), "]: ");
    for (auto& net : high_fanout_nets)
    {
        PSN_LOG_DEBUG("Net:", handler.name(net), handler.fanoutCount(net));
    }
    auto clock_pins = handler.clockPins();

    int create_buffer_count = 0;

    std::vector<int> current_buffer;
    for (auto& net : high_fanout_nets)
    {
        InstanceTerm* source_pin   = handler.faninPin(net);
        bool          is_top_level = false;
        if (!source_pin)
        {
            source_pin   = handler.port(handler.name(net).c_str());
            is_top_level = true;
        }
        if (source_pin && !clock_pins.count(source_pin))
        {
            PSN_LOG_DEBUG("Buffering:", handler.name(net));
            auto fanout_pins        = handler.fanoutPins(net);
            int  net_sink_pin_count = fanout_pins.size();
            PSN_LOG_DEBUG("Sink count:", net_sink_pin_count);
            int              iter = net_sink_pin_count;
            std::vector<int> buffer_hier;
            while (iter > max_fanout)
            {
                iter = (iter * 1.0) / max_fanout;
                buffer_hier.push_back(ceil(iter));
            }
            handler.disconnectAll(net);
            if (!is_top_level)
            { // Top level ports are not disconnected by disconnect all.
                handler.connect(net, source_pin);
            }

            int current_sink_count = 0;
            int levels             = buffer_hier.size();
            if (levels)
            {
                for (int i = 0; i < levels; i++)
                {
                    current_buffer.push_back(0);
                }

                while (current_sink_count < net_sink_pin_count)
                {

                    if (current_buffer[current_buffer.size() - 1] == 0)
                    {
                        for (int i = 1; i < levels; i++)
                        {
                            std::vector<int> parent_buf(current_buffer.begin(),
                                                        current_buffer.end() -
                                                            i);
                            if (name_to_buffer[bufferName(parent_buf)] ==
                                nullptr)
                            {
                                auto buf_name = bufferName(parent_buf);
                                auto net_name = bufferNetName(parent_buf);

                                Instance* new_buffer = handler.createInstance(
                                    handler
                                        .generateInstanceName("psn_fo_buff_",
                                                              buff_index)
                                        .c_str(),
                                    cell);
                                name_to_buffer[buf_name] = new_buffer;
                                create_buffer_count++;
                                Net* new_net = handler.createNet(
                                    handler.generateNetName(net_index).c_str());
                                name_to_net[net_name] = new_net;
                                if (!new_buffer)
                                {
                                    PSN_LOG_CRITICAL(
                                        "Failed to create buffer instance",
                                        buf_name,
                                        ", "
                                        "cannot recover the design, you may "
                                        "need "
                                        "to restart the flow.");
                                }
                                if (!new_net)
                                {
                                    PSN_LOG_CRITICAL("Failed to create net",
                                                     net_name,
                                                     ", "
                                                     "cannot recover the "
                                                     "design, you may need "
                                                     "to restart the flow.");
                                }
                                if (new_buffer && new_net)
                                {
                                    handler.connect(new_net, new_buffer,
                                                    cell_out_pin);
                                    if (i == levels - 1)
                                    {
                                        handler.connect(net, new_buffer,
                                                        cell_in_pin);
                                    }
                                    else
                                    {
                                        auto parent_buf_net_name =
                                            bufferNetName(std::vector<int>(
                                                parent_buf.begin(),
                                                parent_buf.end() - 1));
                                        auto parent_net =
                                            name_to_net[parent_buf_net_name];
                                        if (!parent_net)
                                        {
                                            PSN_LOG_CRITICAL(
                                                "Failed to find net",
                                                parent_buf_net_name,
                                                ", "
                                                "cannot recover the "
                                                "design, you may need "
                                                "to restart the flow.");
                                        }
                                        else
                                        {
                                            handler.connect(parent_net,
                                                            new_buffer,
                                                            cell_in_pin);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    auto buf_name = bufferName(current_buffer);
                    auto net_name = bufferNetName(current_buffer);

                    Instance* new_buffer = handler.createInstance(
                        handler.generateInstanceName("psn_fo_buff_", buff_index)
                            .c_str(),
                        cell);
                    name_to_buffer[buf_name] = new_buffer;
                    create_buffer_count++;
                    Net* new_net = handler.createNet(
                        handler.generateNetName(net_index).c_str());
                    name_to_net[net_name] = new_net;

                    if (!new_buffer)
                    {
                        PSN_LOG_CRITICAL(
                            "Failed to create buffer", buf_name,
                            ", "
                            "cannot recover the design, you may need "
                            "to restart the flow.");
                    }
                    if (!new_net)
                    {
                        PSN_LOG_CRITICAL("Failed to create net", net_name,
                                         ", cannot recover the design, you may "
                                         "need to restart the flow.");
                    }
                    if (new_buffer && new_net)
                    {
                        handler.connect(new_net, new_buffer, cell_out_pin);
                        int sink_connect_count =
                            std::min(max_fanout,
                                     net_sink_pin_count - current_sink_count);
                        for (int i = 0; i < sink_connect_count; i++)
                        {
                            handler.connect(new_net,
                                            fanout_pins[current_sink_count]);
                            current_sink_count++;
                        }
                        if (levels == 1)
                        {
                            handler.connect(net, new_buffer, cell_in_pin);
                        }
                        else
                        {
                            auto current_buf_net_name = bufferNetName(
                                std::vector<int>(current_buffer.begin(),
                                                 current_buffer.end() - 1));
                            auto current_net =
                                name_to_net[current_buf_net_name];
                            if (current_net)
                            {
                                handler.connect(current_net, new_buffer,
                                                cell_in_pin);
                            }
                            else
                            {
                                PSN_LOG_CRITICAL(
                                    "Failed to find net", current_buf_net_name,
                                    ",  cannot recover the design, you may "
                                    "need to restart the flow.");
                            }
                        }
                    }
                    current_buffer = nextBuffer(current_buffer, max_fanout);
                }
            }
        }
    }
    psn_inst->handler()->notifyDesignAreaChanged();
    PSN_LOG_INFO("Added", create_buffer_count, "buffers");

    return create_buffer_count;
}
std::vector<int>
BufferFanoutTransform::nextBuffer(std::vector<int> current_buffer,
                                  int              max_fanout)
{
    int              levels    = current_buffer.size();
    int              incr_done = 0;
    std::vector<int> next_buffer;
    for (int i = levels - 1; i >= 0; i--)
    {
        if (incr_done)
        {
            next_buffer.insert(next_buffer.begin(), current_buffer[i]);
        }
        else if (current_buffer[i] == max_fanout - 1)
        {
            next_buffer.insert(next_buffer.begin(), 0);
        }
        else
        {
            next_buffer.insert(next_buffer.begin(), current_buffer[i] + 1);
            incr_done = 1;
        }
    }
    return next_buffer;
}
std::string
BufferFanoutTransform::bufferName(int index)
{
    return std::string("psn_fo_buf_" + std::to_string(index) + "_");
}

std::string
BufferFanoutTransform::bufferNetName(int index)
{
    return std::string("psn_fo_bufnet_" + std::to_string(index) + "_");
}
std::string
BufferFanoutTransform::bufferName(std::vector<int> indices)
{
    std::string name;
    for (unsigned int i = 0; i < indices.size(); i++)
    {
        name += std::to_string(indices[i]);
        if (i != indices.size() - 1)
        {
            name += "_";
        }
    }
    return std::string("psn_fo_buf_" + name);
}

std::string
BufferFanoutTransform::bufferNetName(std::vector<int> indices)
{
    std::string name;
    for (unsigned int i = 0; i < indices.size(); i++)
    {
        name += std::to_string(indices[i]);
        if (i != indices.size() - 1)
        {
            name += "_";
        }
    }
    return std::string("psn_fo_bufnet_" + name);
}

int
BufferFanoutTransform::run(Psn* psn_inst, std::vector<std::string> args)
{

    if (args.size() == 2 && StringUtils::isNumber(args[0].c_str()))
    {
        return buffer(psn_inst, stoi(args[0]), args[1]);
    }
    else
    {
        PSN_LOG_ERROR(help());
    }

    return -1;
}
} // namespace psn