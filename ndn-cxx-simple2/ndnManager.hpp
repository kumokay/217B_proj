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

#ifndef NDN_MANAGER_HPP
#define NDN_MANAGER_HPP

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <iostream>
#include "ndnRealApp.hpp"

namespace app {

#define CLASSNAME ndnManager

#define IS_SIMULATION 1

class CLASSNAME : public ndnRealApp
{
public:
  CLASSNAME(std::string AppId, ndn::KeyChain& keyChain) :
    ndnRealApp(AppId, keyChain)
  {
  }

  void
  initApp(std::string child_prefix)//, int child_number)
  {
    m_child_prefix = child_prefix;  // /UnderAgent1, /UnderAgent2 ,etc
#if !(__IS_SIMULATION__)
    ndnRealApp::createIdentity("/manager/keyAdmin");
    ndnRealApp::createIdentity("/manager/dataAdmin");
    ndnRealApp::setDefaultIdentity("/manager/dataAdmin");
#endif
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
    APP_LOG(DEBUG, "onInterest: " << name);

    /* available interest filters
       - /source/function1
       - /Manager1/source/function1
    */
    if(name.at(0).toUri() == "source")
    {
      // handle the case /source/function1
      std::string target_task = name.at(1).toUri();
      if (isTaskFileExists(target_task))
      {
        // reply source name: /Manager/source/function1
        ndn::Name source_name(m_nodeName);
        source_name.append(name.getSubName(0));
        std::string content = source_name.toUri();
        ndnRealApp::sendData(name, content, 10000); // 10 sec
      }
    }
    else if(name.getSubName(0,2).toUri() == m_nodeName + "/source")
    {
      // handle the case /Manager/source/function1
      std::string target_task = name.at(2).toUri();
      APP_LOG(DEBUG, "check if taskFile exists:" << target_task);
      if (isTaskFileExists(target_task))
      {
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
            APP_LOG(ERROR, "onInterest: "<< name << ", seg_id >  finalBlockId=" << finalBlockId);
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
        APP_LOG(DEBUG, "sent data name=" << name);
      }
    }
  }

  std::string gen_taskFilePath(std::string task_name)
  {
    return "/home/kumokay/github/ndnSIM/ns-3/src/ndnSIM/examples/ndn-cxx-simple2"
        + m_nodeName + "." + task_name + ".out"; // Manager.function1.out
  }

  bool isTaskFileExists (const std::string target_task)
  {
    if (m_taskLibrary.find(target_task) != m_taskLibrary.end())
    {
      APP_LOG(DEBUG, "task already exists: " << target_task);
      return true;
    }
    else
    {
      std::string file_path = gen_taskFilePath(target_task);
      APP_LOG(DEBUG, "check if file exists:" << file_path);
      std::ifstream ifs(file_path);
      if (ifs.good())
      {
        m_taskLibrary[target_task] = true;
        APP_LOG(DEBUG, "file exists: " << file_path);
        return true;
      }
    }
    APP_LOG(DEBUG, "file does not exists");
    return false;
  }

  int genData(std::string target_task, char** memblock, unsigned int* memblock_sz) // memblock should delete by caller
  {
    std::string file_path = gen_taskFilePath(target_task); // /.../Manager.function1
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
  std::unordered_map<std::string, bool> m_taskLibrary; // task_name, true
  std::unordered_map<ndn::Name, std::vector<std::string>> m_taskTmpContent;  // data_name, content
};

} // namespace app

#undef CLASSNAME

#endif // NDN_HOUSE_INFO_APP_HPP
