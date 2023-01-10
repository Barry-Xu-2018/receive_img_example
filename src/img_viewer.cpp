#include "cista.h"

#include <atomic>
#include <condition_variable>
#include <memory.h>
#include <memory>
#include <mutex>
#include <thread>

#include "include/input_param_parser.hpp"
#include "include/img_msg.hpp"
#include "include/mqtt_subscription.hpp"
#include "include/msg_queue.hpp"

// Exit flag
std::atomic_bool g_request_exit{false};
std::mutex g_main_thread_mutex;
std::condition_variable g_main_thread_cond;

static void signal_handler(int signal)
{
  g_request_exit = true;
  g_main_thread_cond.notify_one();
}

static void
data_process(std::shared_ptr<MqttSubscription> sub,
             std::shared_ptr<MsgQueue<std::vector<uint8_t>>> queue) {
  std::printf("data_process thread start !!!\n");
  sub->init();
  while(!g_request_exit) {
    auto serialized_msg = queue->get_msg_from_queue();
    if (serialized_msg.get() == nullptr) {
      break;
    }

    auto deserialized_msg =
        cista::deserialize<imx500_img_transport::img_msg>(*serialized_msg);

    std::printf("%ld: %d x %d\n", deserialized_msg->timestamp,
                deserialized_msg->width, deserialized_msg->width);
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

  return 0;
}