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

#ifndef NDN_AGENT_HPP
#define NDN_AGENT_HPP

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <boost/thread/thread.hpp>
#include <boost/chrono.hpp>

#include <iostream>
#include "ndnRealApp.hpp"

namespace app {

#define CLASSNAME ndnAgent

class CLASSNAME : public ndnRealApp
{
public:
  CLASSNAME(std::string AppId, ndn::KeyChain& keyChain) :
    ndnRealApp(AppId, keyChain)
  {
  }

  void
  initApp(std::string parent_prefix, std::string child_prefix, int exec_time)//, int child_number)
  {
    m_parent_prefix = parent_prefix;  // /UnderManager1, UnderManager2 ,etc
    m_child_prefix = child_prefix;  // /UnderAgent1, /UnderAgent2 ,etc
    m_exec_time = exec_time;
  }

  void
  setInterestFilter(const ndn::InterestFilter& interestFilter)
  {
    ndnRealApp::setInterestFilter(interestFilter, std::bind(&CLASSNAME::onInterest, this, _1, _2));
  }

  void
  scheduleEvent_ExpressInterest(const ndn::Name& name, int after_ms)
  {
    ndnRealApp::scheduleEvent_ExpressInterest(name, after_ms, std::bind(&CLASSNAME::onData, this, _1, _2));
  }

  void
  run()
  {
    processEvents();
  }

private:
  void
  onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest)
  {
    STAT_LOG_PKT(INTEREST, IN, interest);
    ndn::Name name = interest.getName();
    APP_LOG(TRACE, "onInterest: " << name);

    /* available interest filters
       - /available/task/function1
       - /available/source/function1
       - /Agent1/source/function1
       manages tasks
       - /UnderAgent1/function1
       - /UnderAgent1/function2...
       for manager contact
       - /Agent1/Manager1/install/function1
    */
    if(name.getSubName(0,2).toUri() == m_nodeName + m_parent_prefix)
    {
      // /Agent1/Manager1/install/function1
      APP_LOG(DEBUG, "install task from parent");
      std::string command = name.at(2).toUri();
      if (command == "install")
      {
        std::string task_name = name.at(3).toUri();
        if (m_taskLibrary.find(task_name) != m_taskLibrary.end())
        {
          APP_LOG(DEBUG, "installing or already installted");
          return;
        }
        m_taskLibrary[task_name] = -1;
        // try to fetch /Manager1/source/function1/0
        ndn::Name next_name(m_parent_prefix +"/source/" + task_name);
        auto func_ref = std::bind(&CLASSNAME::onData_sourceContent, this, _1, _2);
        REGISTER_CALLBACK_RETREIVE_ALL_SEGMENTS(next_name, func_ref);
        // response directly
        ndnRealApp::sendData(name, "ACK: " + command, 10000); // 10 sec
      }
      else
      {
        ndnRealApp::sendData(name, "NACK: no such command" + command, 10000); // 10 sec
      }
    }
    else if (name.getSubName(0, 2).toUri() == "/available/task")
    {
      // handle the case /available/task/function1/<TS>/<Latency>
      APP_LOG(TRACE, "check if task available");
      std::string target_task = name.at(2).toUri();
      int latency = std::stoi(name.at(-1).toUri());
      int sent_ts = std::stoi(name.at(-2).toUri());
      int current_ts = getCurrentTS();

      if (m_taskLibrary.find(target_task) != m_taskLibrary.end())
      {
        // check if worth to reply
        int exec_time = m_taskLibrary[target_task];
        if (exec_time == -1)
        {
          APP_LOG(TRACE, "task not ready yet");
        }
        else if ( 2*(current_ts - sent_ts) + exec_time < latency)
        {
          // reply task name: /UnderAgent1/function1
          ndn::Name task_name(m_child_prefix);
          task_name.append(name.getSubName(2));
          std::string content = task_name.toUri();
          ndnRealApp::sendData(name, content, 10000); // 10 sec
        }
      }
      else
      {
        if ( 2*(current_ts - sent_ts) < latency)
        {
          m_taskLibrary[target_task] = -1;
          // try to find /available/source/function1/
          ndn::Name next_name("/available/source/" + target_task);
          ndnRealApp::scheduleEvent_ExpressInterest(next_name, 0,
              std::bind(&CLASSNAME::onData_sourceName, this, _1, _2));
        }
      }
    }
    else if(name.getSubName(0, 2).toUri() == "/available/source")
    {
      // handle the case /available/source/function1
      APP_LOG(TRACE, "check if source available");
      std::string target_task = name.at(2).toUri();
      if (m_taskLibrary.find(target_task) != m_taskLibrary.end())
      {
        if (m_taskLibrary[target_task] == -1)
        {
          APP_LOG(DEBUG, "task source not ready");
        }
        else
        {
          // reply source name: /Agent1/source/function1
          ndn::Name source_name(m_nodeName);
          source_name.append(name.getSubName(1));
          std::string content = source_name.toUri();
          ndnRealApp::sendData(name, content, 10000); // 10 sec
        }
      }
    }
    else if(name.getSubName(0, 2).toUri() == m_nodeName + "/source")
    {
      // handle the case /Agent1/source/function1
      APP_LOG(DEBUG, "send source code to consumer");
      std::string target_task = name.at(2).toUri();
      if (m_taskLibrary.find(target_task) != m_taskLibrary.end())
      {
        if (m_taskLibrary[target_task] == -1)
        {
          APP_LOG(ERROR, "task source not ready, should not receive download request");
          return;
        }
        // reply source name: /Agent1/source/function1/SEG_ID
        int seg_id = std::stoi(name.at(-1).toUri()); //
        // read source
        char* memblock = NULL;
        unsigned int memblock_sz = 0;
        if (-1 == genData(target_task, &memblock, &memblock_sz))
        {
          APP_LOG(ERROR, "onInterest: "<< name << ", genData() error");
          return;
        }

        if (memblock_sz > 0)
        {
          int finalBlockId = memblock_sz / 8000;
          if (seg_id > finalBlockId)
          {
            APP_LOG(ERROR, "onInterest: "<< name << ", seg_id > finalBlockId=" << finalBlockId);
            return;
          }

          APP_LOG(DEBUG, "onInterest: " << name << ", total data bytes=" << memblock_sz << ", total segments=" << finalBlockId+1);
          int memblock_shift = seg_id == 0 ? 0 : seg_id*8000;
          int byte_to_send = (seg_id == finalBlockId) ? memblock_sz - memblock_shift : 8000;
          ndnRealApp::sendData(name, memblock+memblock_shift, byte_to_send, finalBlockId, 10000); // timeout 10 sec
          APP_LOG(DEBUG, "onInterest: " << name << ", send seg=" << std::to_string(seg_id) << ", size=" << byte_to_send);
          if (memblock != NULL)
            delete[] memblock; // memblock should delete by caller
        }
        else
        {
          ndnRealApp::sendData(name, "NULL", 10000); // timeout 10 sec
        }

      }
    }
    else
    {
      APP_LOG(WARNING, "cannot handle interst: " << name);
    }
  }

  void
  onData(const ndn::Interest& interest, const ndn::Data& data)
  {
    STAT_LOG_PKT(DATA, IN, data);
    APP_LOG(DEBUG, "onData: interest=" << interest << ", data:");
    printData(data);
    validateData(data);
  }

  void
  onData_sourceName(const ndn::Interest& interest, const ndn::Data& data)
  {
    STAT_LOG_PKT(DATA, IN, data);
    APP_LOG(DEBUG, "onData_sourceName: interest=" << interest << ", data:");
    printData(data);
    validateData(data);
    // interest: /source/function1/
    // data: /Agent2/source/function1/
    std::string str_content(data.getContent().value_begin(), data.getContent().value_end());
    ndn::Name next_name(str_content);
    auto func_ref = std::bind(&CLASSNAME::onData_sourceContent, this, _1, _2);
    REGISTER_CALLBACK_RETREIVE_ALL_SEGMENTS(next_name, func_ref);
  }

  void
  onData_sourceContent(const ndn::Interest& interest, const ndn::Data& data)
  {
    STAT_LOG_PKT(DATA, IN, data);
    APP_LOG(DEBUG, "onData_sourceContent: interest=" << interest << ", data:");
    printData(data);
    //validateData(data); THIS IS NOT A READ ON_DATA FUNCTION SO DONT VERIFY
    APP_LOG(DEBUG, "onData_sourceContent: receive all segments");
    // got the whole file
    // save the file, chmod, and exec the file
    ndn::Name name = interest.getName();
    std::string task_name = name.at(2).toUri();
    int exec_time = m_exec_time; // default exec_time
    std::string str_content(data.getContent().value_begin(), data.getContent().value_end());
    deployTask(task_name, str_content, exec_time);
  }

  void
  deployTask(std::string task_name, std::string& task_content, int exec_time)
  {
    APP_LOG(DEBUG, "deployTask: "<< task_name << ", exec_time=" << exec_time << " ms");
    std::string task_file_name = gen_taskFilePath(task_name);
    APP_LOG(DEBUG, "write data to file: "<< task_file_name);
    std::ofstream outfile;
    outfile.open(task_file_name, std::ios::binary | std::ios::out);
    outfile.write(task_content.c_str(), task_content.size());
    outfile.close();

    if (m_taskLibrary.find(task_name) != m_taskLibrary.end())
    {
      if (m_taskLibrary[task_name] == -1)
      {
        m_taskLibrary[task_name] = exec_time;
      }
      else
      {
        APP_LOG(ERROR, "Task already deployed: "<< task_name);
        return;
      }
    }
    else
    {
      APP_LOG(ERROR, "Undefined behavior: "<< task_name);
      return;
    }

    APP_LOG(DEBUG, "Agent enable task filters: "<< task_name);
    /* available interest filters
       - /task/function1
       - /source/function1
       - /Agent1/source/function1 */
    ndnRealApp::setInterestFilter("/available/task/" + task_name, std::bind(&CLASSNAME::onInterest, this, _1, _2));
    ndnRealApp::setInterestFilter("/available/source/" + task_name, std::bind(&CLASSNAME::onInterest, this, _1, _2));
    ndnRealApp::setInterestFilter(m_nodeName + "/source/" + task_name, std::bind(&CLASSNAME::onInterest, this, _1, _2));
#if __IS_SIMULATION__
    // for simulation just enable the interest filter and handle it
    /* managed tasks
       - /UnderAgent1/function1
       - /UnderAgent1/function2... */
    APP_LOG(DEBUG, "enable child task: "<< task_name);
    ndnRealApp::setInterestFilter(m_child_prefix+"/" + task_name, std::bind(&CLASSNAME::onInterest_simulatedTask, this, _1, _2));
#else
    // execute the file
    APP_LOG(DEBUG, "run task: "<< task_file_name);
    // TODO: run real executable
#endif

  }

#if __IS_SIMULATION__
  void
  onInterest_simulatedTask(const ndn::InterestFilter& filter, const ndn::Interest& interest)
  {
    // /UnderAgent1/function1
    STAT_LOG_PKT(INTEREST, IN, interest);
    ndn::Name name = interest.getName();
    APP_LOG(DEBUG, "onInterest: " << name);
    std::string task_name = name.at(1).toUri();
    std::string content = "SimResponse to " + task_name;

    // delay send data
    boost::this_thread::sleep_for(boost::chrono::milliseconds(m_exec_time));
    ndnRealApp::sendData(name, content, 10000);
  }
#endif

  std::string gen_taskFilePath(std::string task_name)
  {
    return "/home/kumokay/github/ndnSIM/ns-3/src/ndnSIM/examples/ndn-cxx-simple2/out"
        + m_nodeName + "." + task_name + ".out"; // Agent1.function1.out
  }

  int genData(std::string target_task, char** memblock, unsigned int* memblock_sz) // memblock should delete by caller
  {
    std::string file_path = gen_taskFilePath(target_task); // /.../Agent1.function1
    std::ifstream file (file_path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
      APP_LOG(DEBUG, "Unable to open file: " << file_path);
      return -1;
    }
    std::streampos size = file.tellg();
    if (size <= 0)
    {
      APP_LOG(DEBUG, "Zero size file: " << file_path);
      return 0;
    }
    char* block = new char [size];
    file.seekg (0, std::ios::beg);
    file.read (block, size);
    file.close();

    *memblock = block;
    *memblock_sz = size;
    APP_LOG(DEBUG, "The entire file content is in memory: " << file_path);
    //delete[] memblock;

    return 0;
  }

private:
  std::string m_child_prefix;
  std::string m_parent_prefix;
  int m_exec_time;
  std::unordered_map<std::string, int> m_taskLibrary; // task_name, exec_time (exec_time = -1: not ready)
  //std::unordered_map<ndn::Name, std::vector<std::string>> m_taskTmpContent;  // data_name, content
};

} // namespace app

#undef CLASSNAME

#endif // NDN_HOUSE_INFO_APP_HPP
