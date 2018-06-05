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

#ifndef NDN_REAL_APP_HPP
#define NDN_REAL_APP_HPP


#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/pib/identity.hpp>
#include <ndn-cxx/security/validator-null.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/encoding/estimator.hpp>
#include <boost/filesystem/fstream.hpp>

#include <ctime>
#include <ratio>
#include <chrono>

#define __IS_SIMULATION__ 1

#if __IS_SIMULATION__
  #define GET_CHRONO_TIMESTAMP_MS(__delay_ms) \
    (ns3::Simulator::Now().GetMilliSeconds() + __delay_ms)
#else
  #define GET_CHRONO_TIMESTAMP_MS(__delay_ms) \
    std::chrono::duration_cast<std::chrono::milliseconds>( \
    (std::chrono::steady_clock::now() + std::chrono::milliseconds(__delay_ms)).time_since_epoch() \
  ).count()
#endif





#define __APP_LOG(x) do{ std::cout << "APP_LOG: [" << m_appId << "]" << x << std::endl; } while(0)

#if 0
  #define __APP_LOG_DEV(x) __APP_LOG("[DEV] " << x)
  #define __APP_LOG_ERROR(x)
  #define __APP_LOG_WARNING(x)
  #define __APP_LOG_INFO(x)
  #define __APP_LOG_DEBUG(x)
  #define __APP_LOG_TRACE(x)
#else
  #define __APP_LOG_DEV(x)
  #define __APP_LOG_ERROR(x) __APP_LOG("[ERROR] " << x)
  #define __APP_LOG_WARNING(x) __APP_LOG("[WARNING] " << x)
  #define __APP_LOG_INFO(x) __APP_LOG("[INFO] " << x)
  #define __APP_LOG_DEBUG(x) __APP_LOG("[DEBUG] " << x)
  #define __APP_LOG_TRACE(x)
#endif
#define APP_LOG(__log_type, x)  if(m_log_on) __APP_LOG_##__log_type("[" << m_nodeName <<"][" << GET_CHRONO_TIMESTAMP_MS(0) << "] " << x)
#define APP_LOG_DELAY(__log_type, x, __delay_ms)  if(m_log_on) __APP_LOG_##__log_type("[" << m_nodeName <<"][" << GET_CHRONO_TIMESTAMP_MS(__delay_ms) << "] " << x)


#define _MOUT m_out

#define __STAT_LOG_START(__file_name) _MOUT.open(__file_name, std::ofstream::out | std::ofstream::trunc)
#define __STAT_LOG_STOP() _MOUT.close()
#define __STAT_LOG_HEADER() _MOUT << "Seq. No\tLog Type\tTime (ms)\tPacketName\tPacketContent\tPacket Type\tDirection\tPacket Size (byte)\n"
#define __STAT_LOG_FOOTER() _MOUT << "Logging stopped at " << GET_CHRONO_TIMESTAMP_MS(0) << "\n"
#define __STAT_LOG_EVENT(__pkt_type, __pkt_dir, __name, __event) do { \
                 m_log_seq_no++; \
                 _MOUT << m_log_seq_no << "\tEVENT\t" << GET_CHRONO_TIMESTAMP_MS(0) << "\t" \
                       << __name << "\t" \
                       << #__pkt_type << "\t" \
                       << #__pkt_dir  << "\t" \
                       << __event << "\t" \
                       << "\n"; \
                 } while(0)

#define STAT_LOG_FUTURE_PKT(__pkt_type, __pkt_dir, __pkt, __delay_ms) do { \
                 m_log_seq_no++; \
                 _MOUT << m_log_seq_no << "\tPKT\t" << (GET_CHRONO_TIMESTAMP_MS(__delay_ms)) << "\t" \
                       << (__pkt).getName() << "\t" \
                       << (__pkt).getName().get(-1) << "\t" \
                       << #__pkt_type << "\t" \
                       << #__pkt_dir  << "\t" \
                       << (__pkt).wireEncode(m_estimator) << "\t" \
                       << "\n"; \
                 } while(0)

#define STAT_LOG_PKT(__pkt_type, __pkt_dir, __pkt) STAT_LOG_FUTURE_PKT(__pkt_type, __pkt_dir, __pkt, 0)

#define STAT_LOG_FLUSH() _MOUT << std::flush


#define REGISTER_CALLBACK_RETREIVE_ALL_SEGMENTS(data_name, funct_ref) \
do { \
  if (m_segmentCallbackMap.find(data_name) == m_segmentCallbackMap.end()) \
  { \
    APP_LOG(DEBUG, "ndnRealApp::register_dataSegment_callback: " << data_name); \
    m_segmentCallbackMap.insert({data_name, funct_ref}); \
    ndnRealApp::scheduleEvent_ExpressInterest_segment(data_name); \
  } \
  else \
    APP_LOG(ERROR, "ndnRealApp::register_dataSegment_callback: entry already existed."); \
} while(0)




namespace app {

#define CLASSNAME ndnRealApp

class CLASSNAME
{
public:

  CLASSNAME(std::string AppId, ndn::KeyChain& keyChain) :
    m_keyChain(keyChain),
    m_appId(AppId),
    m_log_on(true),
    m_app_start_time(0),
    m_nodeName("."),
    m_log_seq_no(0),
    // m_faceConsumer(m_ioService),
    m_faceProducer(m_faceConsumer.getIoService()),
    m_scheduler(m_faceConsumer.getIoService())
  {

  }

  void
  setLogOn(bool is_on)
  {
    m_log_on = is_on;
  }

  void
  stat_logging_start(std::string file_path)
  {
    __STAT_LOG_START(file_path);
    __STAT_LOG_HEADER();
  }

  void
  stat_logging_stop()
  {
    __STAT_LOG_FOOTER();
    __STAT_LOG_STOP();
  }

  void
  setNodeName(std::string name)
  {
    m_nodeName = name;
  }

  void
  setInterestFilter(const ndn::InterestFilter& interestFilter, const ndn::InterestCallback & onInterest_funct)
  {
    // register prefix and set interest filter on producer face
    APP_LOG(DEBUG, "setInterestFilter: " << interestFilter);
    m_faceProducer.setInterestFilter(
      interestFilter,
      onInterest_funct,
      std::bind([this, interestFilter](){
          APP_LOG(DEBUG, "RegisterPrefixSuccessCallback: " << interestFilter);
      }),
      std::bind([this, interestFilter](){
          __STAT_LOG_EVENT(INTEREST, IN, interestFilter, "RegisterPrefixFailure");
          APP_LOG(DEBUG, "RegisterPrefixFailureCallback: " << interestFilter);
      })
    );
  }

  void
  setInterestFilter(const ndn::InterestFilter& interestFilter)
  {
    setInterestFilter(interestFilter, std::bind(&CLASSNAME::onInterest, this, _1, _2));
  }

  void
  scheduleEvent_ExpressInterest(
      const ndn::Name& name, int after_ms,
      const ndn::DataCallback& onData_funct)
  {
    // register prefix and set interest filter on producer face
    // use scheduler to send interest later on consumer face
    APP_LOG(DEBUG, "scheduleEvent_ExpressInterest: " << name);
    if (after_ms > 0)
      APP_LOG_DELAY(DEBUG, "ExpressInterest (delay): " << name, after_ms);
    ndn::Interest interest =  createInterest(name, 10000, true);
    STAT_LOG_FUTURE_PKT(INTEREST, OUT, interest, after_ms);
    m_scheduler.scheduleEvent(
        //ndn::time::nanoseconds(after_ms*1000),
        ndn::time::milliseconds(after_ms),
        [this, name, interest, onData_funct] {
               m_faceConsumer.expressInterest(
                 interest,
                 onData_funct,
                 std::bind([this, name](){
                    __STAT_LOG_EVENT(INTEREST, OUT, name, "Nack");
                    APP_LOG(DEBUG, "NackCallback: " << name);
                 }),
                 std::bind([this, name](){
                   __STAT_LOG_EVENT(INTEREST, OUT, name, "Timeout");
                   APP_LOG(DEBUG, "TimeoutCallback: " << name);
                 })
               );
        }
    );
  }

  void
  scheduleEvent_ExpressInterest(
      const ndn::Name& name, int after_ms)
  {
    scheduleEvent_ExpressInterest(name, after_ms, std::bind(&CLASSNAME::onData, this, _1, _2));
  }

  void
  scheduleEvent_ExpressInterest_withLatency(
      const ndn::Name& name, int after_ms,
      const ndn::DataCallback& onData_funct, int latency)
  {
    ndn::Name new_name(name);
    if (latency > 0)
    {
      new_name.append(std::to_string(GET_CHRONO_TIMESTAMP_MS(after_ms)))
              .append(std::to_string(latency));
    }
    scheduleEvent_ExpressInterest(new_name, after_ms, onData_funct);
  }

  void
  scheduleEvent_ExpressInterest_withLatency(
      const ndn::Name& name, int after_ms, int latency)
  {
    ndn::Name new_name(name);
    if (latency > 0)
    {
      new_name.append(std::to_string(GET_CHRONO_TIMESTAMP_MS(after_ms)))
              .append(std::to_string(latency));
    }
    scheduleEvent_ExpressInterest(new_name, after_ms, std::bind(&CLASSNAME::onData, this, _1, _2));
  }

  void
  processEvents()
  {
    m_faceConsumer.processEvents();
    // m_ioService.run(); ???? idk why i use this
  }

  void run()
  {
    APP_LOG(DEBUG, "ndnRealApp::run(): nothing inside the function");
  }

  //////////// keychain related
  void createIdentity(std::string identityNameStr)
  {
    // create a key pair and set it as the identity's default key
    APP_LOG(DEBUG, "ndnRealApp::createIdentity():" << identityNameStr);
    m_keyChain.createIdentity(ndn::Name(identityNameStr));
  }

  void setDefaultIdentity(std::string identityNameStr)
  {

    APP_LOG(DEBUG, "ndnRealApp::setDefaultIdentity(): old identity" << m_keyChain.getDefaultSigningInfo());
    m_keyChain.setDefaultIdentity(m_keyChain.getPib().getIdentity(ndn::Name(identityNameStr)));
    APP_LOG(DEBUG, "ndnRealApp::setDefaultIdentity(): new identity" << m_keyChain.getDefaultSigningInfo());
  }

  /*ndn::Key getIdentityKey(std::string identityNameStr)
  {
    return m_keyChain.getPib().getIdentity(ndn::Name(identityNameStr)).getDefaultKey();
  }*/

  void validateData(const ndn::Data& data)
  {
    m_validator.validate(
      data,
      std::bind([this, data](){
         APP_LOG(DEBUG, " DataValidationSuccessCallback: " << data.getName());
      }),
      std::bind([this, data](){
         APP_LOG(DEBUG, " DataValidationFailureCallback: " << data.getName());
      })
    );
  }

protected:
  void
  scheduleEvent_ExpressInterest_segment(ndn::Name& data_name)
  {
    data_name.append("0");
    CLASSNAME::scheduleEvent_ExpressInterest(
        data_name, 0, std::bind(&CLASSNAME::onData_segment, this, _1, _2));
  }

  void
  onData_segment(const ndn::Interest& interest, const ndn::Data& data)
  {
    STAT_LOG_PKT(DATA, IN, data);
    APP_LOG(DEBUG, "ndnRealApp::onData_segment: interest=" << interest << ", data:");
    printData(data);
    validateData(data);

    // try to get all segment, perform object recognition, and return a list of objects in the frame
    // interest: /Agent1/source/function1/0
    int seg_id = std::stoi(data.getName().at(-1).toUri());
    int fbi = std::stoi(data.getFinalBlockId().toUri());
    APP_LOG(DEBUG, "ndnRealApp::onData_segment: seg_id = " << seg_id << ", FinalBlockId=" << fbi);

    // save content
    ndn::Name data_name = data.getName().getSubName(0, data.getName().size()-1);  // remove seg id
    APP_LOG(DEBUG, "ndnRealApp::onData_segment: save data content, size=" << data.getContent().size());
    if (m_segmentCallbackMap.find(data_name) == m_segmentCallbackMap.end())
    {
      APP_LOG(ERROR, "ndnRealApp::onData_segment: callback function is not registered.");
      return;
    }
    if (m_segmentMap.find(data_name) == m_segmentMap.end())
    {
      m_segmentMap[data_name] = std::vector<std::string>(fbi+1, "");
    }
    m_segmentMap[data_name][seg_id].assign(data.getContent().value_begin(), data.getContent().value_end());

    if (seg_id < fbi)
    {
      // retrieve more segment
      APP_LOG(DEBUG, "ndnRealApp::onData_segment: retrieve more segment");
      data_name.append(std::to_string(seg_id+1));
      ndnRealApp::scheduleEvent_ExpressInterest(
          data_name, 0, std::bind(&CLASSNAME::onData_segment, this, _1, _2));
    }
    else
    {
      std::vector<std::string>& v_data = m_segmentMap[data_name];
      APP_LOG(DEBUG, "ndnRealApp::onData_segment: receive final block; n_seg = " << v_data.size());
      for (int i=0; i<(int)v_data.size(); i++)
      {
        if (v_data[i] == "")
        {
          // TODO: handle segment lost case
          APP_LOG(DEBUG, "ndnRealApp::onData_segment: did not receive seg "<< i << ", retrive segment");
          return;
        }
      }
      APP_LOG(DEBUG, "ndnRealApp::onData_segment: receive all segments");
      // callback
      if (m_segmentCallbackMap.find(data_name) == m_segmentCallbackMap.end())
      {
        APP_LOG(ERROR, "ndnRealApp::onData_segment: callback_funct not found");
      }
      else
      {
        ndn::DataCallback& callback_funct = m_segmentCallbackMap.at(data_name);
        std::string str_content = "";
        for (int i=0; i<(int)v_data.size(); i++)
          str_content.append(v_data[i]);
        std::shared_ptr<ndn::Data> new_data_ptr = createDataPacket(data_name, str_content, 1000);
        m_keyChain.sign(*new_data_ptr);  // dummy signature
        APP_LOG(DEBUG, "ndnRealApp::onData_segment: send data back through callback_funct");
        callback_funct(interest, *new_data_ptr);
      }
      // erase data by key
      m_segmentMap.erase(data_name);
      m_segmentCallbackMap.erase(data_name);
    }
  }

  void
  sendData(ndn::Name dataName, std::string content, int freshPeriod_ms)
  {
    sendData(dataName, content.c_str(), content.size(), 0, freshPeriod_ms);
  }

  void
  sendData(ndn::Name dataName, const char* content, const int content_sz, int finalBlockId, int freshPeriod_ms)
  {
    std::shared_ptr<ndn::Data> data = createDataPacket(dataName, content, content_sz, finalBlockId, freshPeriod_ms);
    m_keyChain.sign(*data);
    m_faceProducer.put(*data);
    STAT_LOG_PKT(DATA, OUT, *data);
  }

  void
  printData(const ndn::Data& data)
  {
    printData(data, false);
  }

  void
  printData_withContent(const ndn::Data& data)
  {
    printData(data, true);
  }

  void
  printData(const ndn::Data& data, bool is_print_content)
  {
    APP_LOG(DEBUG, "\tName: " << data.getName().toUri() );
    APP_LOG(DEBUG, "\tFreshnessPeriod:" << data.getFreshnessPeriod());
    APP_LOG(DEBUG, "\tFinalBlockId: " << data.getFinalBlockId().toUri() );
    APP_LOG(DEBUG, "\tKeyLocator: " << data.getSignature().getKeyLocator() );
    std::string str_content(data.getContent().value_begin(), data.getContent().value_end());
    if (is_print_content)
    {
      APP_LOG(DEBUG, "\tContent: " << str_content );
    }
    APP_LOG(DEBUG, "\tContent size: " << str_content.size() );
  }

  int getCurrentTS()
  {
    return GET_CHRONO_TIMESTAMP_MS(0);
  }

  void
  onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest)
  {
    STAT_LOG_PKT(INTEREST, IN, interest);
    std::string content = genData(16);
    APP_LOG(DEBUG, "ndnRealApp::onInterest: " << filter << ", send data=" << content);
    sendData(interest.getName(), content, 1000);
  }

  void onData(const ndn::Interest& interest, const ndn::Data& data)
  {
    STAT_LOG_PKT(DATA, IN, data);
    APP_LOG(DEBUG, "ndnRealApp::onData: interest=" << interest << ", data:");
    printData_withContent(data);
    validateData(data);
  }



private:
  std::string
  genData(int size)
  {
    std::string content(size, 0);
    for (int i=0; i<size; ++i) content[i] = rand() % 10 + '0';
    return content;
  }

  std::shared_ptr<ndn::Data>
  createDataPacket(ndn::Name dataName, std::string content, int freshPeriod_ms)
  {
    return createDataPacket(dataName, content.c_str(), content.size(), 0, freshPeriod_ms);
  }

  std::shared_ptr<ndn::Data>
  createDataPacket(ndn::Name dataName, const char* content, const int content_sz, int finalBlockId, int freshPeriod_ms)
  {
    std::shared_ptr<ndn::Data> data = std::make_shared<ndn::Data>();
    data->setName(dataName);
    data->setFreshnessPeriod(ndn::time::milliseconds(freshPeriod_ms));
    data->setContent(reinterpret_cast<const uint8_t*>(content), content_sz);
    data->setFinalBlockId(ndn::Name::Component(std::to_string(finalBlockId)));
    return data;
  }

  ndn::Interest
  createInterest(ndn::Name name, int lifetime, bool mustBeFresh)
  {
    ndn::Interest interest(name);
    interest.setInterestLifetime(ndn::time::milliseconds(lifetime));
    interest.setMustBeFresh(mustBeFresh);

    return interest;
  }

protected:
  // boost::asio::io_service m_ioService;
  ndn::KeyChain& m_keyChain;
  ndn::security::v2::ValidatorNull m_validator;
  std::string m_appId;
  bool m_log_on;
  int m_app_start_time;
  std::string m_nodeName;
  boost::filesystem::ofstream m_out;
  ndn::EncodingEstimator m_estimator;
  unsigned int m_log_seq_no;
  std::unordered_map<ndn::Name, std::vector<std::string>> m_segmentMap;  // data_name, content
  std::unordered_map<ndn::Name, ndn::DataCallback> m_segmentCallbackMap; // data_name, callback
private:
  ndn::Face m_faceConsumer;
  ndn::Face m_faceProducer;
  ndn::Scheduler m_scheduler;
};

#undef CLASSNAME

} // namespace app

#endif // NDN_REAL_APP_HPP
