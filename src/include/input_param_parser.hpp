#ifndef INPUT_PARAM_PARSER_HPP__
#define INPUT_PARAM_PARSER_HPP__

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// This class is used for parsing input command arguments
class InputParamParser final {
public:
  InputParamParser(int argc, char **argv) {
    program_name_ = std::string(argv[0]);
    for (int i = 1; i < argc; ++i) {
      this->param_tokens_.push_back(std::string(argv[i]));
    }
  }

  InputParamParser(const InputParamParser &) = delete;
  InputParamParser(InputParamParser &&) = delete;
  InputParamParser &
  operator=(const InputParamParser &) = delete;
  InputParamParser &&
  operator=(InputParamParser &&) = delete;

  const std::string get_broker_addr() {
    if (cmdOptExists("-a") && !getOneOption("-a").empty()) {
      return getOneOption("-a");
    }

    return std::string();    
  }

  const std::string get_broker_port() {
    if (cmdOptExists("-p") && !getOneOption("-p").empty()) {
      return getOneOption("-p");
    }

    return std::string();    
  }

  const std::string get_topic() {
    if (cmdOptExists("-t") && !getOneOption("-t").empty()) {
      return getOneOption("-t");
    }

    return std::string();    
  }

  void show_usage() {
    std::cout << "Usage: "
      << program_name_ 
      << " -a MQTT_Broker_IP_Addr"
      << " -p Server_TCP_Port"
      << " -t Topic" << std::endl;
  }  

private:
  std::string program_name_;
  std::vector<std::string> param_tokens_;

  const std::string getOneOption(const std::string &opt) const {
    std::vector<std::string>::const_iterator iter;

    iter = std::find(this->param_tokens_.begin(), this->param_tokens_.end(), opt);
    if (iter != this->param_tokens_.end() && ++iter != this->param_tokens_.end()) {
      return *iter;
    }

    return std::string();
  }

  bool cmdOptExists(const std::string &opt) const {
    return std::find(this->param_tokens_.begin(), this->param_tokens_.end(),
                     opt) != this->param_tokens_.end();
  }
};

#endif