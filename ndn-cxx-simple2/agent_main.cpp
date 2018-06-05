/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) Hsaio-Yun Tseng - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 *
 * @author Hsaio-Yun Tseng, tsenghy@g.ucla.edu
 */
#include "ndnAgent.hpp"
#define INSTANCE_CLASS ndnAgent

int
main(int argc, char** argv)
{
  if (argc < 4)
  {
    std::cerr << "USAGE: ./agent_main AGENT_ID PARENT_ID EXEC_TIME" << std::endl;
    return 0;
  }
  std::string m_id = argv[1];
  std::string m_pid = argv[2];
  int m_exec_time = std::stoi(argv[3]);

  std::string m_prefix = "/Agent" + m_id;
  std::string m_child_prefix = "/UnderAgent" + m_id;
  std::string m_parent_prefix = "/Manager" + m_pid;
  std::string m_app_id = "123";
  bool m_is_log_on = true;
  ndn::KeyChain keyChain;
  std::unique_ptr<app::INSTANCE_CLASS> m_instance;
  m_instance.reset(new app::INSTANCE_CLASS(m_app_id, keyChain));
  m_instance->setLogOn(m_is_log_on);
  m_instance->setNodeName(m_prefix);
  m_instance->initApp(m_parent_prefix, m_child_prefix, m_exec_time);//, m_child_number);
  m_instance->setInterestFilter(m_prefix);// + "/cmd_getData");
  m_instance->setInterestFilter("available");// + "/cmd_getData");
  m_instance->stat_logging_start("mylog/agent" + m_id + ".txt");
  m_instance->run();


  return 0;
}
