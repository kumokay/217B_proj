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

#ifndef NDN_APP_STARTER_PRODUCER_HPP
#define NDN_APP_STARTER_PRODUCER_HPP

#include "ndnAppStarter.hpp"
#include "ndnCameraSensor.hpp"

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/application.h"

namespace ns3 {

#define CLASSNAME ndnAppStarterProducer
#define APPNAME app::ndnCameraSensor

// Class inheriting from ns3::Application
class CLASSNAME : public ndnAppStarter
{
public:
  CLASSNAME()
  : m_prefix("")
  {
  }
  static TypeId
  GetTypeId()
  {
    static TypeId tid = CREATE_TYPEID(CLASSNAME)
        .AddAttribute("Prefix", "for setInterestFilter", StringValue("/default"),
                       MakeStringAccessor(&CLASSNAME::m_prefix), MakeStringChecker());
    return tid;
  }
protected:
  // inherited from Application base class.
  virtual void
  StartApplication()
  {
    // Create an instance of the app
    m_instance.reset(new APPNAME(CLASSID(CLASSNAME), ndn::StackHelper::getKeyChain()));
    m_instance->setLogOn(false);
    m_instance->setNodeName(m_prefix);
    m_instance->setInterestFilter(m_prefix + "/cmd_getData");
    m_instance->run();
  }

private:
  string m_prefix;
  std::unique_ptr<APPNAME> m_instance;
};

REG_CLASS(CLASSNAME);

#undef CLASSNAME
#undef APPNAME
//#undef APP_LOG_ON
} // namespace ns3

#endif // NDN_APP_STARTER_PRODUCER_HPP
