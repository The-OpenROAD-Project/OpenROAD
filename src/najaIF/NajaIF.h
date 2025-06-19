
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_CAPNP_H_
#define __SNL_CAPNP_H_

#include <filesystem>
#include <map>
#include <tuple>
#include <string>
//#include <boost/asio.hpp>

namespace odb {

class dbDatabase;
class dbBlock;
class dbModule;

class NajaIF {
  public:
        NajaIF(dbDatabase* db){ db_ = db; }
        ~NajaIF() = default;
    
        // odb::dbDatabase* getDb() const { return db_; }
        // odb::dbBlock* getBlock() const { return block_; }
        // odb::dbBlock* getTopBlock() const { return top_block_; }
    
        // boost::asio::ip::tcp::socket getSocket();
    // boost::asio::ip::tcp::socket getSocket(uint16_t port=0); 
     static constexpr std::string_view InterfaceName = "db_interface.snl";
     static constexpr std::string_view ImplementationName = "db_implementation.snl";
     //void dump(const NLDB* db, const std::filesystem::path& dumpPath);
    // void send(const NLDB* db, const std::string& ipAddress, uint16_t port);
    // void send(const NLDB* db, const std::string& ipAddress, uint16_t port, uint8_t forceDBID);
    //odb::dbDatabase* load(const std::filesystem::path& dumpPath);
    // NLDB* receive(boost::asio::ip::tcp::socket& socket);
    // NLDB* receive(uint16_t port);

    //  void dumpInterface(const NLDB* db, int fileDescriptor);
    //  void dumpInterface(const NLDB* db, int fileDescriptor, uint8_t forceDBID);
    //  void dumpInterface(const NLDB* db, const std::filesystem::path& interfacePath);
    // void sendInterface(const NLDB* db, const std::string& ipAddress, uint16_t port);
    // void sendInterface(const NLDB* db, boost::asio::ip::tcp::socket& socket); 
    // void sendInterface(const NLDB* db, boost::asio::ip::tcp::socket& socket, uint8_t forceDBID); 

     void loadInterface(int fileDescriptor);
     void loadInterface(const std::filesystem::path& interfacePath);
    // NLDB* receiveInterface(uint16_t port);
    // NLDB* receiveInterface(boost::asio::ip::tcp::socket& socket); 

    //  void dumpImplementation(const NLDB* db, int fileDescriptor);
    //  void dumpImplementation(const NLDB* db, int fileDescriptor, uint8_t forceDBID);
    //  void dumpImplementation(const NLDB* db, const std::filesystem::path& implementationPath);
    // void sendImplementation(const NLDB* db, const std::string& ipAddress, uint16_t port);
    // void sendImplementation(const NLDB* db, boost::asio::ip::tcp::socket& socket);
    // void sendImplementation(const NLDB* db, boost::asio::ip::tcp::socket& socket, uint8_t forceDBID);

    void loadImplementation(int fileDescriptor);
    void loadImplementation(const std::filesystem::path& implementationPath);
    // NLDB* receiveImplementation(uint16_t port);
    // NLDB* receiveImplementation(boost::asio::ip::tcp::socket& socket); 

    void makeBlock(const std::string& topName);

//private:
     static std::map<std::tuple<size_t,size_t,size_t>, std::pair<odb::dbModule*, bool>> module_map_;
     static std::map<size_t, std::vector<std::string> > module2terms_; // assuming ID for DBMod is unique
     static odb::dbDatabase* db_;
     static odb::dbBlock* block_;
     static odb::dbBlock* top_block_;
     static odb::dbModule* top_;
};

} // namespace odb

#endif // __SNL_CAPNP_H_
