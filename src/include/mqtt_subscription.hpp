#ifndef MQTT_SUBSCRIPTION_HPP__
#define MQTT_SUBSCRIPTION_HPP__

#include <mosquitto.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>

#include "msg_queue.hpp"

class MqttSubscription final{
public:
  MqttSubscription(std::string broker_ip, int32_t broker_port, std::string topic);
  ~MqttSubscription();

  void init();

  std::string get_topic();

  bool is_connect_broker();
  void update_connect_status(bool is_connected);

  void put_recevied_msg_queue(std::shared_ptr<std::vector<uint8_t>> &msg);

  // May block for waiting msg
  std::shared_ptr<std::vector<uint8_t>> get_msg_from_queue();

  // Break block while get_msg_from_queue()
  void wakeup_for_exit();

private:
  std::string broker_ip_;
  int32_t broker_port_;
  std::string topic_;

  std::atomic_bool is_connected_{false};

  std::mutex queue_mutex_;
  MsgQueue<std::vector<uint8_t>> queue_;

  struct mosquitto * mosq_{nullptr};

  static void on_connect(struct mosquitto *mosq, void *obj, int reason_code);
  static void on_subscribe(struct mosquitto *mosq, void *obj, int mid,
                           int qos_count, const int *granted_qos);
  static void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);
};

#endif