/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) Hsaio-Yun Tseng - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 *
 * @author Hsaio-Yun Tseng, tsenghy@g.ucla.edu
 */
#include "ndnRealApp.hpp"
#include "ndnSensorSetting.hpp"
#define INSTANCE_CLASS ndnRealApp

std::string number2id(int id)
{
  std::string number_str = std::to_string(id);
  if (number_str.size() < 4) //e.g. 1 -> 0001
    number_str = std::string(4 - number_str.size(), '0') + number_str;
  return number_str;
}

int
main(int argc, char** argv)
{

  std::string m_interestPrefix = "/NESL/Event/city_c/area_a/house_h/sensor_s/cmd_getRangeData/0001-0010";
  std::string m_app_id = "123";
  bool m_is_log_on = true;
  ndn::KeyChain keyChain;
  std::unique_ptr<app::INSTANCE_CLASS> m_instance;
  m_instance.reset(new app::INSTANCE_CLASS(m_app_id, keyChain));
  m_instance->setNodeName("consumer_" + std::to_string(N_SENSORS));
  m_instance->setLogOn(m_is_log_on);
  m_instance->stat_logging_start("mylog/consumer.txt");

  {
    const int n_sensors = N_SENSORS;
    const int n_houses = N_HOUSES;
    const int n_areas = N_AREAS;
    const int n_cities = N_CITIES;

    int frame_start = 1;
    const int frames_per_interval = 5;
    const int interval = 5; // request every N seconds
    for (int t=1; t<=10; t++)
    {
      std::string frame_id1 = number2id(frame_start);
      std::string frame_id2 = number2id(frame_start+frames_per_interval-1);
      frame_start += 1;

      for (int c=0; c<n_cities; c++)
        for (int a=0; a<n_areas; a++)
          for (int h=0; h<n_houses; h++)
            for (int s=0; s<n_sensors; s++)
            {
                //"/NESL/Event/city_0/area_1/house_2/sensor_3/cmd_getRangeData/0001-0005"
                std::string prefix = "/NESL/Event/city_" + std::to_string(c) + "/area_" + std::to_string(a) + "/house_" + std::to_string(h) + "/sensor_" + number2id(s)
                    + "/cmd_getRangeData/" + frame_id1 + "-" + frame_id2;
                m_instance->scheduleEvent_ExpressInterest(prefix, ndn::time::seconds(t*interval));
            }
    }
    /*m_instance->scheduleEvent_ExpressInterest(m_interestPrefix, ndn::time::seconds(7));
    m_instance->scheduleEvent_ExpressInterest(m_interestPrefix, ndn::time::seconds(9));
    m_instance->scheduleEvent_ExpressInterest(m_interestPrefix, ndn::time::seconds(11));
    m_instance->scheduleEvent_ExpressInterest(m_interestPrefix, ndn::time::seconds(12));
    m_instance->scheduleEvent_ExpressInterest(m_interestPrefix, ndn::time::seconds(13));*/
  }

  m_instance->run();

  return 0;
}
