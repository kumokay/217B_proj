/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) Hsaio-Yun Tseng - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 *
 * @author Hsaio-Yun Tseng, tsenghy@g.ucla.edu
 */

#include "ndnConsumer.hpp"
#define INSTANCE_CLASS ndnConsumer

int
main(int argc, char** argv)
{
  std::string m_interestPrefix = "";
  int m_latency = 0;

  switch(argc)
  {
    case 3:
      m_latency = std::stoi(argv[2]);
    case 2:
      m_interestPrefix = argv[1];
      break;
    default:
      std::cerr << "USAGE: ./anyconsumer_main INTEREST_PREFIX [latency]" << std::endl;
      return 0;
  }
  std::string m_app_id = "123";
  bool m_is_log_on = true;
  ndn::KeyChain keyChain;
  std::unique_ptr<app::INSTANCE_CLASS> m_instance;
  m_instance.reset(new app::INSTANCE_CLASS(m_app_id, keyChain));
  m_instance->setNodeName("anyconsumer");
  m_instance->setLogOn(m_is_log_on);
  m_instance->stat_logging_start("mylog/anyconsumer.txt");

  // express interest
  if (m_latency > 0)
  {
    m_instance->scheduleEvent_ExpressInterest_withLatency(m_interestPrefix, ndn::time::seconds(0), m_latency);
  }
  else
    m_instance->scheduleEvent_ExpressInterest(m_interestPrefix, ndn::time::seconds(0));

  m_instance->run();

  return 0;
}
