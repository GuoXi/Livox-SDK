//
// The MIT License (MIT)
//
// Copyright (c) 2019 Livox. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "data_handler.h"
#include <base/logging.h>
#include "hub_data_handler.h"
#include "lidar_data_handler.h"

namespace livox {

static const size_t kSphericalCoordinatePointSize = 9;
static const size_t kCartesianCoordinatePointSize = 13;
static const size_t kPrefixDataSize = 18;

DataHandler &data_handler() {
  static DataHandler handler;
  return handler;
}

#pragma pack(1)

#pragma pack()

bool DataHandler::AddDataListener(uint8_t handle, const DataCallback &cb) {
  if (handle >= callbacks_.size()) {
    return false;
  }
  callbacks_[handle] = cb;
  return true;
}

bool DataHandler::AddDevice(const DeviceInfo &info) {
  if (impl_ == NULL) {
    DeviceMode mode = static_cast<DeviceMode>(device_manager().device_mode());
    if (mode == kDeviceModeHub) {
      impl_.reset(new HubDataHandlerImpl(this, mem_pool_));
    } else if (mode == kDeviceModeLidar) {
      impl_.reset(new LidarDataHandlerImpl(this, mem_pool_));
    }

    if (impl_ == NULL || !impl_->Init()) {
      impl_.reset(NULL);
      return false;
    }
  }
  return impl_->AddDevice(info);
}

bool DataHandler::Init() {
  apr_status_t rv = apr_pool_create(&mem_pool_, NULL);
  if (rv != APR_SUCCESS) {
    return false;
  }
  return true;
}

void DataHandler::Uninit() {
  if (impl_) {
    impl_.reset(NULL);
  }
  if (mem_pool_) {
    apr_pool_destroy(mem_pool_);
    mem_pool_ = NULL;
  }
}

void DataHandler::OnDataCallback(uint8_t handle, void *data, uint16_t size) {
  LivoxEthPacket *lidar_data = (LivoxEthPacket *)data;
  if (lidar_data == NULL) {
    return;
  }
  if (handle >= callbacks_.size()) {
    return;
  }
  DataCallback cb = callbacks_[handle];
  if (lidar_data->data_type == 0) {
    size = (size - kPrefixDataSize) / kCartesianCoordinatePointSize;
  } else if (lidar_data->data_type == 1) {
    size = (size - kPrefixDataSize) / kSphericalCoordinatePointSize;
  } else {
    return;
  }
  if (cb) {
    //    LOG_INFO << " device_sn: " <<  device_sn << std::endl;
    //    LOG_INFO << " version: " << (uint32_t) lidar_data_cb->version << std::endl;
    //    LOG_INFO << " slot: " << (uint32_t) lidar_data_cb->slot << std::endl;
    //    LOG_INFO << " resverd : " << (uint32_t) lidar_data_cb->reserved << std::endl;
    //    LOG_INFO << " errorCode: " << (uint32_t) lidar_data_cb->err_code << std::endl;
    //    LOG_INFO << " lidarID: " << (uint16_t) lidar_data_cb->id << std::endl;
    //    LOG_INFO << " timeStampType: " << (uint32_t) (lidar_data_cb->timestamp_type) << std::endl;
    //    LOG_INFO << " timeStamp: " << (uint32_t) *(lidar_data_cb->time_stamp) << std::endl;
    //    LOG_INFO << " dataType: " << (uint16_t) lidar_data->data_type << std::endl << std::endl;
    cb(handle, lidar_data, size);
  }
}

void DataHandler::RemoveDevice(uint8_t handle) {
  if (impl_) {
    impl_->RemoveDevice(handle);
  }
}

}  // namespace livox
