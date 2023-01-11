#include "include/mqtt_subscription.hpp"

MqttSubscription::MqttSubscription(
    std::string broker_ip, int32_t broker_port, std::string topic,
    std::shared_ptr<MsgQueue<std::vector<uint8_t>>> &queue)
    : broker_ip_(broker_ip), broker_port_(broker_port), topic_(topic),
      queue_(queue) {}

MqttSubscription::~MqttSubscription() {
  mosquitto_lib_cleanup();
  mosquitto_destroy(mosq_);
}

void MqttSubscription::init() {

	int rc;

	/* Required before calling other mosquitto functions */
	mosquitto_lib_init();

	/* Create a new client instance.
	 * id = NULL -> ask the broker to generate a client id for us
	 * clean session = true -> the broker should remove old sessions when we connect
	 * obj = NULL -> we aren't passing any of our private data for callbacks
	 */
	mosq_ = mosquitto_new(NULL, true, static_cast<void *>(this));
	if(mosq_ == NULL){
		fprintf(stderr, "Error: Out of memory.\n");
		return;
	}

	/* Configure callbacks. This should be done before connecting ideally. */
	mosquitto_connect_callback_set(mosq_, on_connect);
	mosquitto_subscribe_callback_set(mosq_, on_subscribe);
	mosquitto_message_callback_set(mosq_, on_message);

	/* Connect to test.mosquitto.org on port 1883, with a keepalive of 60 seconds.
	 * This call makes the socket connection only, it does not complete the MQTT
	 * CONNECT/CONNACK flow, you should use mosquitto_loop_start() or
	 * mosquitto_loop_forever() for processing net traffic. */
	rc = mosquitto_connect(mosq_, broker_ip_.c_str(), broker_port_, 60);
	if(rc != MOSQ_ERR_SUCCESS){
		mosquitto_destroy(mosq_);
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		return;
	}

  /* Run the network loop in a background thread, this call returns quickly. */
  rc = mosquitto_loop_start(mosq_);
  if(rc != MOSQ_ERR_SUCCESS){
    mosquitto_destroy(mosq_);
    std::printf("Error: %s\n", mosquitto_strerror(rc));
    return;
  }
}

bool MqttSubscription::is_connect_broker() {
  return is_connected_;
}

void MqttSubscription::update_connect_status(bool is_connected) {
  is_connected_ = is_connected;
}

std::string MqttSubscription::get_topic() {
  return topic_;
}

std::shared_ptr<MsgQueue<std::vector<uint8_t>>> MqttSubscription::get_msg_queue() {
	return queue_;
}

/* Callback called when the client receives a CONNACK message from the broker. */
void MqttSubscription::on_connect(struct mosquitto *mosq, void *obj, int reason_code) {
	int rc;
  auto instance = static_cast<MqttSubscription *>(obj);
	/* Print out the connection result. mosquitto_connack_string() produces an
	 * appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
	 * clients is mosquitto_reason_string().
	 */
	std::printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
	if(reason_code != 0){
		/* If the connection fails for any reason, we don't want to keep on
		 * retrying in this example, so disconnect. Without this, the client
		 * will attempt to reconnect. */
    std::printf("Connection failed !\n");
		mosquitto_disconnect(mosq);
    instance->update_connect_status(false);
    return;
	}

  instance->update_connect_status(true);

	/* Making subscriptions in the on_connect() callback means that if the
	 * connection drops and is automatically resumed by the client, then the
	 * subscriptions will be recreated when the client reconnects. */
	rc = mosquitto_subscribe(mosq, NULL, instance->get_topic().c_str(), 1);
	if(rc != MOSQ_ERR_SUCCESS){
		fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
		/* We might as well disconnect if we were unable to subscribe */
    std::printf("Subscribe failed !\n");
		mosquitto_disconnect(mosq);
    instance->update_connect_status(false);
	}  
}

/* Callback called when the broker sends a SUBACK in response to a SUBSCRIBE. */
void MqttSubscription::on_subscribe(struct mosquitto *mosq, void *obj, int mid,
                                    int qos_count, const int *granted_qos)
{
  bool have_subscription = false;
  auto instance = static_cast<MqttSubscription *>(obj);

	/* In this example we only subscribe to a single topic at once, but a
	 * SUBSCRIBE can contain many topics at once, so this is one way to check
	 * them all. */
	for(int i=0; i<qos_count; i++){
		std::printf("on_subscribe: %d:granted qos = %d\n", i, granted_qos[i]);
		if(granted_qos[i] <= 2){
			have_subscription = true;
		}
	}
	if(have_subscription == false){
		/* The broker rejected all of our subscriptions, we know we only sent
		 * the one SUBSCRIBE, so there is no point remaining connected. */
		std::printf("Error: All subscriptions rejected.\n");
		mosquitto_disconnect(mosq);
    instance->update_connect_status(false);
	}
}

/* Callback called when the client receives a message. */
void MqttSubscription::on_message(struct mosquitto *mosq, void *obj,
                                  const struct mosquitto_message *msg)
{
  /* This blindly prints the payload, but the payload can be anything so take care. */
	//printf("%s %d %s\n", msg->topic, msg->qos, (char *)msg->payload);

  auto instance = static_cast<MqttSubscription *>(obj);

  auto serialized_data = std::make_shared<std::vector<uint8_t>>(
      static_cast<uint8_t *>(msg->payload),
      static_cast<uint8_t *>(msg->payload) + msg->payloadlen);

  instance->get_msg_queue()->add_msg_to_queue(serialized_data);
}
