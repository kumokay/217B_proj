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

#ifndef NDN_EVENT_APP_HPP
#define NDN_EVENT_APP_HPP

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <iostream>
#include <unordered_map>
#include <vector>
#include <bitset>
#include "ndnRealApp.hpp"

namespace app {

#define CLASSNAME ndnEventApp

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

  void printDataStructure()
  {
    //std::unordered_map<std::string, std::pair<int, unsigned int>> Map_uri;
    //std::unordered_map<std::string, std::vector<std::string>> Map_frameId;

    for ( auto it = Map_uri.begin(); it != Map_uri.end(); ++it )
      APP_LOG(DEBUG, "Map_uri: " << it->first << " : " << it->second.first << "," << std::bitset<32>(it->second.second));

    for ( auto it = Map_frameId.begin(); it != Map_frameId.end(); ++it )
      APP_LOG(DEBUG, "Map_frameId: " << it->first << " : " << it->second.size());
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
      // /NESL/Event/city_c/area_a/house_h/sensor_s/cmd_getRangeData/FRAMEID1-FRAMEID2
      // /NESL/Object/city_c/area_a/house_h/sensor_s/cmd_getRangeData/FRAMEID1-FRAMEID2
      ndn::Name next_name("/NESL/Object");
      next_name.append(name.getSubName(2,6));
      ndnRealApp::scheduleEvent_ExpressInterest(next_name, ndn::time::nanoseconds(0),
                                                std::bind(&CLASSNAME::onData_query, this, _1, _2));
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
    // interest: /NESL/Object/city_0/area_1/house_2/sensor_3/cmd_getRangeData/0840-0850
    // data: /0840/0,/0841/0,/0842/0,/0843/0,/0844/0,/0845/0,/0846/0,/0847/0,/0848/0,/0849/0,/0850/0,

    ndn::Name name = interest.getName();
    std::string frame_range = name.at(7).toUri();
    std::size_t found = frame_range.find('-');
    int frame_start = std::stoi(frame_range.substr(0, found-0));
    int frame_end = std::stoi(frame_range.substr(found+1));
    APP_LOG(DEBUG, "onData_query: " << name << ", frame_range=" << frame_start << "-" << frame_end);


    // add target frames and uri to map
    std::string map_uri_key = data.getName().getSubName(2,6).toUri(); // /city_0/area_1/house_2/sensor_3/cmd_getRangeData/0840-0850
    if (Map_uri.find(map_uri_key) == Map_uri.end())
    {
      unsigned int unrecvframe_bitmap = 0;
      for (int i=frame_start; i<=frame_end; i++)
      {
        unrecvframe_bitmap = (unrecvframe_bitmap << 1) | 1;
      }
      APP_LOG(DEV, "Map_uri.insert");
      Map_uri.insert(std::make_pair(map_uri_key, std::make_pair(frame_start, unrecvframe_bitmap)));
    }

    // parse content and add frame_ids to map
    //std::unordered_map<int, std::vector<std::string>> Map_frameId; //frame_id, <FRAMEID1-FRAMEID2, FRAMEID1-FRAMEID2, ...>
    std::string str_content(data.getContent().value_begin(), data.getContent().value_end());
    std::size_t begin = 0;
    do {
      std::size_t del_pos = str_content.find_first_of(',', begin);
      std::string frame_uri = str_content.substr(begin, del_pos-begin); // /0840/0,
      begin = del_pos+1;
      APP_LOG(DEV, "frame_uri=" << frame_uri);

      // /NESL/Object/city_0/area_1/house_2/sensor_3/cmd_getData/0840/0
      // ..
      // /NESL/Object/city_0/area_1/house_2/sensor_3/cmd_getData/0850/0
      ndn::Name next_name = interest.getName().getSubName(0, 6); // /NESL/Object/city_0/area_1/house_2/sensor_3
      next_name.append("/cmd_getData" + frame_uri); // /cmd_getData/0840/0

      std::string map_frameId_key = next_name.getSubName(2,7).toUri(); // /city_0/area_1/house_2/sensor_3/cmd_getData/0840/0
      if (Map_frameId.find(map_frameId_key) == Map_frameId.end())
      {
        std::vector<std::string> v;
        v.push_back(map_uri_key);
        APP_LOG(DEV, "Map_frameId.insert");
        Map_frameId.insert(std::make_pair(map_frameId_key, v));
      }
      else
      {
        APP_LOG(DEV, "Map_frameId.push_back");
        Map_frameId[map_frameId_key].push_back(map_uri_key);
      }

      // schedule event to get all data
      ndnRealApp::scheduleEvent_ExpressInterest(next_name, ndn::time::nanoseconds(0),
                                                std::bind(&CLASSNAME::onData_producer, this, _1, _2));
    } while (begin < str_content.size());
  }

  void
  onData_producer(const ndn::Interest& interest, const ndn::Data& data)
  {
    STAT_LOG_PKT(DATA, IN, data);
    APP_LOG(DEBUG, "onData_producer: interest=" << interest << ", data:");
    printData(data);
    APP_LOG(DEV, "onData_producer: recv data" << data.getName().toUri());
    // /NESL/Object/city_0/area_1/house_2/sensor_3/cmd_getData/0840/0
    int frame_id = std::stoi(data.getName().at(7).toUri());
    std::string map_frameId_key = data.getName().getSubName(2,7).toUri(); // /city_0/area_1/house_2/sensor_3/cmd_getData/0840/0
    if (Map_frameId.find(map_frameId_key) != Map_frameId.end())
    {
      std::vector<std::string> & v = Map_frameId[map_frameId_key];
      for (int i=0; i<(int)v.size(); i++)
      {
        std::string & map_uri_key = v[i];
        // update unrecv_bitmap
        if (Map_uri.find(map_uri_key) != Map_uri.end())
        {
          int & frame_start = Map_uri[map_uri_key].first;
          unsigned int & unrecvframe_bitmap = Map_uri[map_uri_key].second;
          // unmask the corresponding bit
          unsigned int bit_mask = 1 << (frame_id - frame_start);
          APP_LOG(DEV, "receive frame_id=" << frame_id << ", update bitmap=" << std::bitset<32>(unrecvframe_bitmap) << " & " << std::bitset<32>(~bit_mask));
          unrecvframe_bitmap = unrecvframe_bitmap & (~bit_mask);
          if (unrecvframe_bitmap == 0)
          {
            // all frames are received, remove entry from Map_uri and response to the original interest
            APP_LOG(DEV, "Map_uri.erase");
            Map_uri.erase(map_uri_key);

            // /NESL/Event/city_c/area_a/house_h/sensor_s/cmd_getRangeData/FRAMEID1-FRAMEID2
            ndn::Name name("/NESL/Event");
            name.append(map_uri_key);
            // fake result for now cuz the surveillance video we used is super dull, nothing special happened
            ndnRealApp::sendData(name, "Normal", 10000); // timeout 10 sec
          }
        }

        // frame received and result updated, remove entry from Map_frameId
        APP_LOG(DEV, "Map_frameId.erase");
        Map_frameId.erase(map_frameId_key);
      }
    }
  }

private:
  std::string child_prefix;
  std::unordered_map<std::string, std::pair<int, unsigned int>> Map_uri;
      //</city_c/area_a/house_h/sensor_s/cmd_getRangeData/FRAMEID1-FRAMEID2, <frame_number, recvframe_bitmap>>
  std::unordered_map<std::string, std::vector<std::string>> Map_frameId;
      //</city_c/area_a/house_h/sensor_s/cmd_getData/FRAMEID/0, </city_c/area_a/house_h/sensor_s/cmd_getRangeData/FRAMEID1-FRAMEID2, FRAMEID1-FRAMEID2, ...>>
  //////////////////////////////////////////////////////////////////////////
  // TODO: any manipulation to Map_uri, Map_frameId
  //       should be protected by critical sections or something like that
  //       but i am running out of time so leave it as a todo for now
  //////////////////////////////////////////////////////////////////////////
};

} // namespace app

#undef CLASSNAME

#endif // NDN_HOUSE_INFO_APP_HPP
