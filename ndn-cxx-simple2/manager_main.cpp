/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) Hsaio-Yun Tseng - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 *
 * @author Hsaio-Yun Tseng, tsenghy@g.ucla.edu
 */
#include "ndnManager.hpp"
#define INSTANCE_CLASS ndnManager

int
main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::cerr << "USAGE: ./manager_main MANAGER_ID" << std::endl;
    return 0;
  }
  std::string m_id = argv[1];
  std::string m_prefix = "/Manager" + m_id;
  std::string m_child_prefix = "/UnderManager" + m_id;
  std::string m_app_id = "123";
  bool m_is_log_on = true;
  ndn::KeyChain keyChain;
  std::unique_ptr<app::INSTANCE_CLASS> m_instance;
  m_instance.reset(new app::INSTANCE_CLASS(m_app_id, keyChain));
  m_instance->setLogOn(m_is_log_on);
  m_instance->setNodeName(m_prefix);
  m_instance->initApp(m_child_prefix);//, m_child_number);
  m_instance->setInterestFilter(m_prefix);// + "/cmd_getData");
  m_instance->stat_logging_start("mylog/manager" + m_id + ".txt");
  m_instance->run();

  return 0;
}
