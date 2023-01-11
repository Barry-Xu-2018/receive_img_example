#include "cista.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory.h>
#include <memory>
#include <mutex>
#include <opencv2/core/hal/interface.h>
#include <opencv2/core/types.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <thread>

#include <opencv2/opencv.hpp>

#include "include/input_param_parser.hpp"
#include "include/img_msg.hpp"
#include "include/mqtt_subscription.hpp"
#include "include/msg_queue.hpp"

// Exit flag
std::atomic_bool g_request_exit{false};
std::mutex g_main_thread_mutex;
std::condition_variable g_main_thread_cond;

std::atomic_int64_t g_last_timestamp = 0;

static void signal_handler(int signal)
{
  g_request_exit = true;
  g_main_thread_cond.notify_one();
}

static void show_empty_window() {
  cv::Mat empty_frame = cv::Mat::zeros(300, 300, CV_8UC3);
  cv::putText(empty_frame, "Wait for BMP ...", cv::Point(20, 150),
              cv::FONT_HERSHEY_COMPLEX, 1, cv::Scalar(0, 0, 255), 1);
  cv::imshow("Visualizer", empty_frame);
  cv::waitKey(1);
}

static void
data_process(std::shared_ptr<MqttSubscription> sub,
             std::shared_ptr<MsgQueue<std::vector<uint8_t>>> queue) {
  std::printf("data_process thread start !!!\n");
  sub->init();
  show_empty_window();
  while(!g_request_exit) {
    auto serialized_msg = queue->get_msg_from_queue();
    if (serialized_msg.get() == nullptr) {
      break;
    }

    auto deserialized_msg =
        cista::deserialize<imx500_img_transport::img_msg>(*serialized_msg);

    if (deserialized_msg->encoding == "rgb8") {
      cv::Mat colors[4];
      uint32_t pictureSize = deserialized_msg->height * deserialized_msg->width;

      colors[2] = cv::Mat(deserialized_msg->height,
                          deserialized_msg->width, CV_8UC1,
                          (reinterpret_cast<uint8_t *>(&deserialized_msg->data[0])));
      colors[1] = cv::Mat(deserialized_msg->height,
                          deserialized_msg->width, CV_8UC1,
                          (reinterpret_cast<uint8_t *>(
                               &deserialized_msg->data[0]) +
                           pictureSize));
      colors[0] = cv::Mat(deserialized_msg->height,
                          deserialized_msg->width, CV_8UC1,
                          (reinterpret_cast<uint8_t *>(
                               &deserialized_msg->data[0]) +
                           pictureSize * 2));
      cv::Mat resizeWin;
      uint8_t merge_num = imx500_img_transport::numChannels(deserialized_msg->encoding.str());
      cv::merge(colors, merge_num, resizeWin);
      cv::Rect roi(cv::Point(0, 0), cv::Size(deserialized_msg->width, deserialized_msg->height));

      cv::imshow("Visualizer", resizeWin(roi));
      g_last_timestamp = deserialized_msg->timestamp;
      cv::waitKey(2);
    } else {
      std::cout << "Unsupported encoding " << deserialized_msg->encoding << " !!!" << std::endl;
    }
  }

  std::printf("data_process thread exit !!!\n");
}

int main(int argc, char ** argv)
{
  auto parser = std::make_shared<InputParamParser>(argc, argv);

  std::string mqtt_broker_ip = parser->get_broker_addr();
  if (mqtt_broker_ip.empty()) {
    std::cout << "Input command arguments \"-a\" error !" << std::endl;
    parser->show_usage();
    return EXIT_FAILURE;
  }

  std::string mqtt_broker_port = parser->get_broker_port();
  if (mqtt_broker_port.empty()) {
    std::cout << "Input command arguments \"-p\" error !" << std::endl;
    parser->show_usage();
    return EXIT_FAILURE;
  }

  int32_t broker_port;
  try {
    broker_port = std::stoi(mqtt_broker_port, nullptr);
  } catch (std::invalid_argument) {
    std::cout << "Input command arguments \"-p\" error !" << std::endl;
    parser->show_usage();
    return EXIT_FAILURE;
  }

  std::string topic = parser->get_topic();
  if (topic.empty()) {
    std::cout << "Input command arguments \"-t\" error !" << std::endl;
    parser->show_usage();
    return EXIT_FAILURE;
  }

  std::cout << "Input parameter:" << std::endl;
  std::cout << "    Broker IP: " << mqtt_broker_ip << std::endl;
  std::cout << "  Broker port: " << broker_port << std::endl;
  std::cout << "        Topic: " << topic << std::endl;

  auto msg_queue = std::make_shared<MsgQueue<std::vector<uint8_t>>>();

  auto sub =
      std::make_shared<MqttSubscription>(mqtt_broker_ip, broker_port, topic, msg_queue);

  auto deserialized_data_thread =
      std::make_shared<std::thread>(data_process, sub, msg_queue);

  auto updated_window = std::make_shared<std::thread>([](){
    static std::atomic_int64_t save_timestamp = 0;
    while (!g_request_exit) {
      if (save_timestamp == g_last_timestamp) {
        show_empty_window();
        std::this_thread::sleep_for(std::chrono::seconds(1));
      } else {
        save_timestamp.exchange(g_last_timestamp);
      }
    }
  });

  // main thread enter wait status
  if (!g_request_exit) {
    std::unique_lock<std::mutex> lock(g_main_thread_mutex);
    g_main_thread_cond.wait(lock, []{
      return g_request_exit == true;
    });
  }
  msg_queue->wakeup_for_exit();

  if (deserialized_data_thread->joinable()) {
    deserialized_data_thread->joinable();
  }

  if(updated_window->joinable()) {
    updated_window->join();
  }

  cv::destroyAllWindows();

  return 0;
}