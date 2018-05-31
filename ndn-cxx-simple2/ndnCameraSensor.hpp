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

#ifndef NDN_CAMERA_SENSOR_HPP
#define NDN_CAMERA_SENSOR_HPP

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <iostream>
#include <fstream>
#include "ndnRealApp.hpp"
//#include "ndnSurvInfoDef.hpp"

namespace app {

#define CLASSNAME ndnCameraSensor

class CLASSNAME : public ndnRealApp
{
public:
  CLASSNAME(std::string AppId, ndn::KeyChain& keyChain) :
    ndnRealApp(AppId, keyChain)
  {
  }

  void
  setInterestFilter(const ndn::InterestFilter& interestFilter)
  {
    ndnRealApp::setInterestFilter(interestFilter, std::bind(&ndnCameraSensor::onInterest, this, _1, _2));
  }

private:
  void
  onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest)
  {
    ndn::Name name = interest.getName(); // /NESL/Producer/city_c/area_a/house_h/sensor_s/cmd_getData/FRAME_ID/SEG_ID
    APP_LOG(DEBUG, "onInterest: " << name);
    std::string frame_id = name.at(7).toUri();
    int seg_id = std::stoi(name.at(8).toUri());
    APP_LOG(DEBUG, "onInterest: " << filter << ", frame_id=" << frame_id << ", seg_id=" << seg_id);
    //std::string content = genData();
    char* memblock = NULL;
    unsigned int memblock_sz = 0;
    if (-1 == genData(frame_id, &memblock, &memblock_sz))
    {
      APP_LOG(ERROR, "onInterest: genData() error");
      return;
    }

    int finalBlockId = memblock_sz / 8000;
    APP_LOG(DEBUG, "onInterest: " << name << ", total data bytes=" << memblock_sz << ", total segments=" << finalBlockId+1);
    int memblock_shift = seg_id == 0 ? 0 : seg_id*8000;
    int byte_to_send = (seg_id == finalBlockId) ? memblock_sz - memblock_shift : 8000;
    ndnRealApp::sendData(name, memblock+memblock_shift, byte_to_send, finalBlockId, 10000); // timeout 10 sec
    APP_LOG(DEBUG, "onInterest: " << name << ", send seg=" << std::to_string(seg_id) << ", size=" << byte_to_send);
    //cout << "onInterest: " << name << ", send seg=" << to_string(seg_id) << ", size=" << byte_to_send << endl;

    if (memblock != NULL)
      delete[] memblock; // memblock should delete by caller
  }

  void run()
  {
    processEvents();
  }

  int genData(std::string frame_id, char** memblock, unsigned int* memblock_sz) // memblock should delete by caller
  {
    std::string file_path = "/home/kumokay/yolo/darknet/visor_data/mov0/frames/" + frame_id + ".jpg";
    std::ifstream file (file_path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
      APP_LOG(DEBUG, "Unable to open file: " << file_path);
      return -1;
    }
    std::streampos size = file.tellg();
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

  /*static std::string
  genData()
  {
    int n_objects = rand() % 10; // 0~9 object in one place
    std::string content = "";
    for (int i=0; i<n_objects; ++i)
    {
      int obj_id = rand() % eObjSIZE;
      int act_id = rand() % num_actions[obj_id];
      content += objects[obj_id] + ":" + actions[obj_id][act_id] + ",";
    }
    return content;
  }*/

private:

};

#undef CLASSNAME

} // namespace app

#endif // NDN_CAMERA_SENSOR_HPP
