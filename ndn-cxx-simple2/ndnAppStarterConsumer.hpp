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

#include "ndnRealApp.hpp"

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/application.h"
#include "ndnSensorSetting.hpp"

namespace ns3 {

#define CLASSNAME ndnAppStarterConsumer
#define APPNAME app::ndnRealApp

// Class inheriting from ns3::Application
class CLASSNAME : public ndnAppStarter
{
public:
  static TypeId
  GetTypeId()
  {
    static TypeId tid = CREATE_TYPEID(CLASSNAME)
        .AddAttribute("InterestPrefix", "for ExpressInterest", StringValue("/defaultInterest"),
                       MakeStringAccessor(&CLASSNAME::m_interestPrefix), MakeStringChecker());
    return tid;
  }
protected:
  // inherited from Application base class.
  virtual void
  StartApplication()
  { 
    // Create an instance of the app
    m_instance.reset(new APPNAME(CLASSID(CLASSNAME), ndn::StackHelper::getKeyChain()));
    m_instance->setNodeName("server for " + m_interestPrefix);
    m_instance->setLogOn(false);
    m_instance->stat_logging_start("mylog/consumer.txt");
    m_instance->run();
    /* queries for test
    m_instance->scheduleEvent_ExpressInterest(m_interestPrefix + "/0001/0", ndn::time::seconds(1));
    m_instance->scheduleEvent_ExpressInterest(m_interestPrefix + "/0002/0", ndn::time::seconds(3));
    m_instance->scheduleEvent_ExpressInterest(m_interestPrefix + "/0003/0", ndn::time::seconds(5));
    m_instance->scheduleEvent_ExpressInterest(m_interestPrefix + "/0004/0", ndn::time::seconds(5));
    m_instance->scheduleEvent_ExpressInterest(m_interestPrefix + "/0005/0", ndn::time::seconds(5));
    m_instance->scheduleEvent_ExpressInterest(m_interestPrefix + "/0006/0", ndn::time::seconds(5));
    m_instance->scheduleEvent_ExpressInterest(m_interestPrefix + "/0840/0", ndn::time::seconds(8));
    m_instance->scheduleEvent_ExpressInterest("/NESL/Query/city_0/area_1/house_2/sensor_3/cmd_getRangeData/0840-0850", ndn::time::seconds(10));
    m_instance->scheduleEvent_ExpressInterest("/NESL/Object/city_0/area_1/house_2/sensor_3/cmd_getData/0847/0", ndn::time::seconds(11)); 
    m_instance->scheduleEvent_ExpressInterest("/NESL/Event/city_0/area_1/house_2/sensor_3/cmd_getRangeData/0820-0830", ndn::time::seconds(13)); */
    /*
    m_instance->scheduleEvent_ExpressInterest("/NESL/Event/city_0/area_1/house_2/sensor_3/cmd_getRangeData/0001-0005", ndn::time::seconds(1));
    m_instance->scheduleEvent_ExpressInterest("/NESL/Event/city_0/area_1/house_2/sensor_3/cmd_getRangeData/0011-0015", ndn::time::seconds(2));
    m_instance->scheduleEvent_ExpressInterest("/NESL/Event/city_0/area_1/house_2/sensor_3/cmd_getRangeData/0021-0025", ndn::time::seconds(3));
    m_instance->scheduleEvent_ExpressInterest("/NESL/Event/city_0/area_1/house_2/sensor_3/cmd_getRangeData/0031-0035", ndn::time::seconds(4));
    m_instance->scheduleEvent_ExpressInterest("/NESL/Event/city_0/area_1/house_2/sensor_3/cmd_getRangeData/0041-0045", ndn::time::seconds(5));
    */
    // create home sensors and routers
    const int n_sensors = N_SENSORS; // 8 sensors connected to router
    const int n_houses = N_HOUSES;
    const int n_areas = N_AREAS;
    const int n_cities = N_CITIES;
    
    int frame_start = 1;
    const int frames_per_second = 5;
    const int GOP_sec = 5;
    for (int t=6; t<=15; t++)
    {
      std::string frame_id1 = create_frameId(frame_start);
      std::string frame_id2 = create_frameId(frame_start+frames_per_second*GOP_sec-1);
      frame_start += frames_per_second;
      
      for (int c=0; c<n_cities; c++)
        for (int a=0; a<n_areas; a++)
          for (int h=0; h<n_houses; h++)
            for (int s=0; s<n_sensors; s++)
            {
                //"/NESL/Event/city_0/area_1/house_2/sensor_3/cmd_getRangeData/0001-0005"
                std::string prefix = "/NESL/Event/city_" + to_string(c) + "/area_" + to_string(a) + "/house_" + to_string(h) + "/sensor_" + to_string(s) 
                    + "/cmd_getRangeData/" + frame_id1 + "-" + frame_id2;
                m_instance->scheduleEvent_ExpressInterest(prefix, ndn::time::seconds(t));
            }
    }
    /*m_instance->scheduleEvent_ExpressInterest(m_interestPrefix, ndn::time::seconds(7));
    m_instance->scheduleEvent_ExpressInterest(m_interestPrefix, ndn::time::seconds(9));
    m_instance->scheduleEvent_ExpressInterest(m_interestPrefix, ndn::time::seconds(11));
    m_instance->scheduleEvent_ExpressInterest(m_interestPrefix, ndn::time::seconds(12));
    m_instance->scheduleEvent_ExpressInterest(m_interestPrefix, ndn::time::seconds(13));*/
    m_instance->processEvents();
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
  string m_interestPrefix;
  std::unique_ptr<APPNAME> m_instance; 

};

REG_CLASS(CLASSNAME);

#undef CLASSNAME
#undef APPNAME
} // namespace ns3

#endif // NDN_APP_STARTER_CONSUMER_HPP
