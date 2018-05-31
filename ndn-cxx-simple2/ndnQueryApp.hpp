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

#ifndef NDN_QUERY_APP_HPP
#define NDN_QUERY_APP_HPP

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <iostream>
#include <unordered_map>
#include <vector>
#include "ndnRealApp.hpp"
//#include "ndnSurvInfoDef.hpp"

namespace app {

#define CLASSNAME ndnQueryApp

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
    // /NESL/Query/city_c/area_a/house_h/sensor_s/cmd_getRangeData/FRAMEID1-FRAMEID2
    STAT_LOG_PKT(INTEREST, IN, interest);
    ndn::Name name = interest.getName();
    std::string frame_range = name.at(7).toUri();
    std::size_t found = frame_range.find('-');
    int frame_start = std::stoi(frame_range.substr(0, found));
    int frame_end = std::stoi(frame_range.substr(found+1));
    APP_LOG(DEBUG, "onInterest: " << name << ", frame_range=" << frame_start << "-" << frame_end);

    ndn::Name next_name(child_prefix);
    next_name.append(name.getSubName(2,5));
    std::string content;
    for (int i=frame_start; i<=frame_end; i++)
    {
      // /NESL/Producer/city_c/area_a/house_h/sensor_s/cmd_getData/0001/0
      //                                                       FRAME_ID SEGMENT
      std::string frame_id = std::to_string(i);
      if (frame_id.size() < 4) //e.g. 0001
        frame_id = std::string(4 - frame_id.size(), '0') + frame_id;
      content += "/"+frame_id+"/0,";
    }
    APP_LOG(DEBUG, "onInterest: " << interest.getName() << ", send data=" << content);
    ndnRealApp::sendData(interest.getName(), content, 10000);

  }

  void
  onData(const ndn::Interest& interest, const ndn::Data& data)
  {
    STAT_LOG_PKT(DATA, IN, data);
    APP_LOG(DEBUG, "onData: interest=" << interest << ", data:");
    printData(data);
  }

private:
  std::string child_prefix;
};

} // namespace app

#undef CLASSNAME

#endif // NDN_HOUSE_INFO_APP_HPP
