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

#ifndef NDN_APP_STARTER_CONSUMER_HPP
#define NDN_APP_STARTER_CONSUMER_HPP

#include "ndnTaskConsumer.hpp"

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/application.h"

namespace ns3 {

#define CLASSNAME ndnAppStarterConsumer
#define APPNAME app::ndnTaskConsumer

// Class inheriting from ns3::Application
class CLASSNAME : public ndnAppStarter
{
public:
  static TypeId
  GetTypeId()
  {
    static TypeId tid = CREATE_TYPEID(CLASSNAME)
        .AddAttribute("Prefix", "for Prefix", StringValue("/defaultPrefix"),
                       MakeStringAccessor(&CLASSNAME::m_prefix), MakeStringChecker());
    return tid;
  }
protected:
  // inherited from Application base class.
  virtual void
  StartApplication()
  {
    string log_folder = "out/log/";
    if(__IS_SIMULATION__)
      log_folder = "src/ndnSIM/examples/my-ndn-simple2-log/";
    // Create an instance of the app
    m_instance.reset(new APPNAME(CLASSID(CLASSNAME), ndn::StackHelper::getKeyChain()));
    m_instance->setNodeName(m_prefix);
    m_instance->setLogOn(true);
    m_instance->stat_logging_start(log_folder + "consumer.txt");

    // queries for test
    m_instance->scheduleEvent_ExpressInterest("/Agent0/Manager0/install/simTask", 3000);
    for (int i=0; i<30; i++)
    {
      m_instance->scheduleEvent_ExpressInterest_withLatency(
          "/available/task/simTask", 6000+i*100, 500);
      //m_instance->processEvents();
    }
    m_instance->run();

  }

  virtual void
  StopApplication()
  {
    // Stop and destroy the instance of the app
    m_instance->stat_logging_stop();
  }

  string create_frameId(int id)
  {
    std::string frame_id = std::to_string(id);
    if (frame_id.size() < 4) //e.g. 0001
      frame_id = string(4 - frame_id.size(), '0') + frame_id;
    return frame_id;
  }

private:
  string m_prefix;
  std::unique_ptr<APPNAME> m_instance;

};

REG_CLASS(CLASSNAME);

#undef CLASSNAME
#undef APPNAME
} // namespace ns3

#endif // NDN_APP_STARTER_CONSUMER_HPP
