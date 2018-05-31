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

#ifndef NDN_APP_STARTER_HPP
#define NDN_APP_STARTER_HPP

#include "ndnRealApp.hpp"

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/application.h"


namespace ns3 {

#define __STRINGIZE(X) #X
#define CLASSID(CLASS)   __STRINGIZE(CLASS)
#define REG_CLASS(CLASS) NS_OBJECT_ENSURE_REGISTERED(CLASS)
#define CREATE_TYPEID(CLASS) TypeId(CLASSID(CLASS)).SetParent<Application>().AddConstructor<CLASS>()

#define CLASSNAME ndnAppStarter

// Class inheriting from ns3::Application
class CLASSNAME : public Application
{
public:
  static TypeId
  GetTypeId()
  {
    static TypeId tid = CREATE_TYPEID(CLASSNAME);
    return tid;
  }
protected:
  // inherited from Application base class.
  virtual void
  StartApplication()
  {
    // Create an instance of the app
    _m_instance.reset(new app::ndnRealApp(CLASSID(CLASSNAME), ndn::StackHelper::getKeyChain()));
    _m_instance->setInterestFilter("/hello/world");
    _m_instance->run(); // can be omitted
  }

  virtual void
  StopApplication()
  {
    // Stop and destroy the instance of the app
    _m_instance.reset();
  }

private:
   std::unique_ptr<app::ndnRealApp> _m_instance;
};

REG_CLASS(CLASSNAME);

#undef CLASSNAME

} // namespace ns3

#endif // NDN_APP_STARTER_HPP
