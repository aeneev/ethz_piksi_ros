#include "piksi_multi_cpp/receiver/receiver.h"

#include <libsbp/sbp.h>
#include <piksi_rtk_msgs/Heartbeat.h>

// SBP message definitions.
#include <libsbp/system.h>

namespace piksi_multi_cpp {

std::vector<Receiver::ReceiverType> Receiver::kTypeVec =
    std::vector<Receiver::ReceiverType>(
        {kBaseStationReceiver, kPositionReceiver, kAttitudeReceiver, kUnknown});

Receiver::Receiver(const ros::NodeHandle& nh, const Device::DevicePtr& device)
    : nh_(nh), device_(device), is_running_(true) {
  // Initialize SBP state.
  state_ = std::make_shared<sbp_state_t>();
  sbp_state_init(state_.get());

  // Register callbacks.
  cb_.push_back(SBPCallback::create(nh, SBP_MSG_HEARTBEAT, state_));
}

Receiver::~Receiver() {
  // Close thread.
  is_running_.store(false);
  process_thread_.join();

  if (device_) device_->close();
}

bool Receiver::init() {
  if (!device_.get()) {
    ROS_ERROR("Device not set.");
    return false;
  }

  // Open attached device.
  if (!device_->open()) {
    ROS_ERROR("Cannot open device.");
    return false;
  }

  process_thread_ = std::thread(&Receiver::process, this);

  return true;
}

void Receiver::process() {
  while (is_running_.load()) {
    if (!device_.get()) return;
    // Pass device pointer to process function.
    sbp_state_set_io_context(state_.get(), device_.get());
    // Pass device read function to sbp_process.
    int result =
        sbp_process(state_.get(), &piksi_multi_cpp::Device::read_redirect);
    if (result < 0) {
      ROS_WARN_STREAM("Error sbp_process: " << result);
    }
  }
}

}  // namespace piksi_multi_cpp