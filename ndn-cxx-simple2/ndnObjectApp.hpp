/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#ifndef NDN_OBJECT_APP_HPP
#define NDN_OBJECT_APP_HPP

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <iostream>
#include "ndnRealApp.hpp"

namespace app {

#define CLASSNAME ndnObjectApp

class CLASSNAME : public ndnRealApp
{
public:
  CLASSNAME(std::string AppId, ndn::KeyChain& keyChain) :
    ndnRealApp(AppId, keyChain)
  {
  }

  void
  initApp(std::string child_prefix)//, int child_number)
  {
    this->child_prefix = child_prefix;
    //this->child_number = child_number;
    //this->child_mask = 0;
    //for(int i=0; i<child_number; ++i)
    //  this->child_mask |= (1 << i);
  }

  void
  setInterestFilter(const ndn::InterestFilter& interestFilter)
  {
    ndnRealApp::setInterestFilter(interestFilter, std::bind(&CLASSNAME::onInterest, this, _1, _2));
  }

  void
  scheduleEvent_ExpressInterest(const ndn::Name& name, const ndn::time::nanoseconds& after)
  {
    ndnRealApp::scheduleEvent_ExpressInterest(name, after, std::bind(&CLASSNAME::onData, this, _1, _2));
  }

  void
  run()
  {
  }

private:
  void
  onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest)
  {
    STAT_LOG_PKT(INTEREST, IN, interest);
    ndn::Name name = interest.getName();
    APP_LOG(DEBUG, "onInterest: " << name);
    if (name.at(6).toUri() == "cmd_getRangeData")
    {
      // /NESL/Object/city_c/area_a/house_h/sensor_s/cmd_getRangeData/FRAMEID1-FRAMEID2
      // /NESL/Query/city_c/area_a/house_h/sensor_s/cmd_getRangeData/FRAMEID1-FRAMEID2
      ndn::Name next_name("/NESL/Query");
      next_name.append(name.getSubName(2,6));
      ndnRealApp::scheduleEvent_ExpressInterest(next_name, ndn::time::nanoseconds(0),
                                                std::bind(&CLASSNAME::onData_query, this, _1, _2));
    }
    else if (name.at(6).toUri() == "cmd_getData")
    {
      // /NESL/Object/city_c/area_a/house_h/sensor_s/cmd_getData/0847/0
      // /NESL/Producer/city_c/area_a/house_h/sensor_s/cmd_getData/0847/0
      ndn::Name next_name("/NESL/Producer");
      next_name.append(name.getSubName(2,7));
      ndnRealApp::scheduleEvent_ExpressInterest(next_name, ndn::time::nanoseconds(0),
                                                std::bind(&CLASSNAME::onData_producer, this, _1, _2));
    }
  }

  void
  onData(const ndn::Interest& interest, const ndn::Data& data)
  {
    STAT_LOG_PKT(DATA, IN, data);
    APP_LOG(DEBUG, "onData: interest=" << interest << ", data:");
    printData(data);
  }

  void
  onData_query(const ndn::Interest& interest, const ndn::Data& data)
  {
    STAT_LOG_PKT(DATA, IN, data);
    APP_LOG(DEBUG, "onData_query: interest=" << interest << ", data:");
    printData(data);
    // interest: /NESL/Query/city_0/area_1/house_2/sensor_3/cmd_getData/0840-0850
    // data: /0840/0,/0841/0,/0842/0,/0843/0,/0844/0,/0845/0,/0846/0,/0847/0,/0848/0,/0849/0,/0850/0,

    // /NESL/Producer/city_0/area_1/house_2/sensor_3/cmd_getData/0840/0
    // ..
    // /NESL/Producer/city_0/area_1/house_2/sensor_3/cmd_getData/0850/0
    std::string str_content(data.getContent().value_begin(), data.getContent().value_end());
    ndn::Name name("/NESL/Object");
    name.append(interest.getName().getSubName(2,6));
    APP_LOG(DEBUG, "response to Interest: " << interest.getName() << ", send data=" << str_content);
    ndnRealApp::sendData(name, str_content, 10000);
  }

  void
  onData_producer(const ndn::Interest& interest, const ndn::Data& data)
  {
    STAT_LOG_PKT(DATA, IN, data);
    APP_LOG(DEBUG, "onData_producer: interest=" << interest << ", data:");
    printData(data);
    // try to get all segment, perform object recognition, and return a list of objects in the frame
    // interest: /NESL/Producer/city_c/area_a/house_h/sensor_s/cmd_getData/0847/0
    APP_LOG(DEV, "recv data interest=" << interest.getName().toUri());
    APP_LOG(DEV, "              data="<< data.getName().toUri());
    if (data.getName().at(8) != data.getFinalBlockId())
    {
      // retrieve more segment
      ndn::Name name = data.getName().getSubName(0, 8); // remove seg id
      name.append(std::to_string(std::stoi(data.getName().at(8).toUri())+1));
      ndnRealApp::scheduleEvent_ExpressInterest(name, ndn::time::nanoseconds(0),
                                                std::bind(&CLASSNAME::onData_producer, this, _1, _2));
    }
    else
    {
      // got the whole frame
      // perform obj recognition and sent object list back
      // original interest: /NESL/Object/city_c/area_a/house_h/sensor_s/cmd_getData/0847/0
      ndn::Name name("/NESL/Object");
      name.append(interest.getName().getSubName(2,6))
          .append("0");

      std::string frame_id = name.at(7).toUri();
      char* memblock = NULL;
      unsigned int memblock_sz = 0;
      if (-1 == genData(frame_id, &memblock, &memblock_sz))
      {
        APP_LOG(DEBUG, "onData_producer: "<< name << ", genData() error");
        return;
      }

      int finalBlockId = 0;
      APP_LOG(DEBUG, "onData_producer: " << name << ", total data bytes=" << memblock_sz << ", total segments=" << finalBlockId+1);

      if (memblock_sz > 0)
      {
        ndnRealApp::sendData(name, memblock, memblock_sz, finalBlockId, 10000); // timeout 10 sec
        if (memblock != NULL)
          delete[] memblock; // memblock should delete by caller
      }
      else
      {
        ndnRealApp::sendData(name, "NULL", 10000); // timeout 10 sec
      }
      APP_LOG(DEV, "send data name=" << name);
    }
  }

  int genData(std::string frame_id, char** memblock, unsigned int* memblock_sz) // memblock should delete by caller
  {
    std::string file_path = "/home/kumokay/yolo/darknet/visor_data/mov0/predictions_txt/" + frame_id + ".jpg.predictions.txt";
    std::ifstream file (file_path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
      APP_LOG(DEBUG, "Unable to open file: " << file_path);
      return -1;
    }
    std::streampos size = file.tellg();
    if (size <= 0)
    {
      APP_LOG(DEBUG, "Zero size file: " << file_path);
      return 0;
    }
    char* block = new char [size];
    file.seekg (0, std::ios::beg);
    file.read (block, size);
    file.close();

    *memblock = block;
    *memblock_sz = size;
    APP_LOG(DEBUG, "The entire file content is in memory: " << file_path);
    //delete[] memblock;

    return 0;
  }

private:
  std::string child_prefix;
};

} // namespace app

#undef CLASSNAME

#endif // NDN_HOUSE_INFO_APP_HPP
