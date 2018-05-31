/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) Hsaio-Yun Tseng - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 *
 * @author Hsaio-Yun Tseng, tsenghy@g.ucla.edu
 */
#include "ndnCameraSensor.hpp"
#define INSTANCE_CLASS ndnCameraSensor

int
main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::cerr << "USAGE: ./camera_main SENSOR_ID" << std::endl;
    return 0;
  }
  std::string m_id = argv[1];
  std::string m_sensor_name = "sensor_" + m_id;
  std::string m_prefix = "/NESL/Producer/city_0/area_0/house_0/" + m_sensor_name + "/cmd_getData";
  bool m_is_log_on = true;
  std::string m_app_id = "123";
  ndn::KeyChain keyChain;
  std::unique_ptr<app::INSTANCE_CLASS> m_instance;
  m_instance.reset(new app::INSTANCE_CLASS(m_app_id, keyChain));
  m_instance->setLogOn(m_is_log_on);
  m_instance->setNodeName(m_sensor_name);
  m_instance->setInterestFilter(m_prefix);
  // m_instance->stat_logging_start("mylog/camera_" + m_sensor_name +".txt");
  m_instance->run(); // can be omitted

  return 0;
}
