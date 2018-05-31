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

#ifndef NDN_CONSUMER_HPP
#define NDN_CONSUMER_HPP

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <iostream>
#include <fstream>
#include "ndnRealApp.hpp"
//#include "ndnSurvInfoDef.hpp"
namespace app {

#define CLASSNAME ndnConsumer

class CLASSNAME : public ndnRealApp
{
public:
  CLASSNAME(std::string AppId, ndn::KeyChain& keyChain) :
    ndnRealApp(AppId, keyChain)
  {
  }

  void run()
  {
    processEvents();
  }

private:
  void
  onData(const ndn::Interest& interest, const ndn::Data& data)
  {
    STAT_LOG_PKT(DATA, IN, data);
    APP_LOG(DEBUG, "onData: interest=" << interest << ", data:");
    printData_withContent(data);
  }
  


};

#undef CLASSNAME

} // namespace app

#endif // NDN_CAMERA_SENSOR_HPP
