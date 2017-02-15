#pragma once

#include <ossia/network/base/listening.hpp>
#include <ossia/network/base/protocol.hpp>
#include <ossia/network/oscquery/detail/client.hpp>
#include <ossia/detail/json.hpp>
#include <readerwriterqueue.h>
namespace osc
{
class sender;
class receiver;
}

namespace oscpack
{
class ReceivedMessage;
class IpEndpointName;
}

namespace ossia
{
namespace oscquery
{

class OSSIA_EXPORT oscquery_mirror_protocol final : public ossia::net::protocol_base
{
public:
  oscquery_mirror_protocol(std::string host, uint16_t local_osc_port = 10203);
  ~oscquery_mirror_protocol();

  bool pull(net::address_base&) override;
  std::future<void> pullAsync(net::address_base&) override;
  void request(net::address_base&) override;
  bool push(const net::address_base&) override;
  bool observe(net::address_base&, bool) override;
  bool observe_quietly(net::address_base&, bool) override;
  bool update(net::node_base& b) override;
  void setDevice(net::device_base& dev) override;
  ossia::net::device_base& getDevice() const { return *m_device; }


  /**
   * @brief Run the commands registered in th event queue
   *
   * The data structures are not protected by locks since it may hurt
   * the performance on a tree, and hard to get right.
   * Instead, all the edition operations on a device should happen on a single thread.
   * When edition operations are received from the network, they are put in a queue.
   *
   * Run this function regularly in order to get the update of the device, for
   * instance in the event loop of your application.
   */
  void runCommands();

  /**
   * @brief Provide a callback for whenever an edition command is received
   *
   * This is an alternative way to calling runCommands() : a function
   * can be provided; it will be called every time a new command is received,
   * after it has been pushed.
   * This can be used to put the command in the application's main loop.
   */
  void setCommandCallback(std::function<void()>);
private:
  using connection_handler = websocketpp::connection_hdl;
  void on_WSMessage(connection_handler hdl, const std::string& message);
  void on_OSCMessage(const oscpack::ReceivedMessage& m, const oscpack::IpEndpointName& ip);

  void cleanup_connections();

  std::unique_ptr<osc::sender> m_oscSender;
  std::unique_ptr<osc::receiver> m_oscServer;

  ossia::oscquery::client m_websocketClient;

  // Listening status of the local software
  net::listened_addresses m_listening;

  ossia::net::device_base* m_device{};

  std::promise<void> m_namespacePromise;

  struct get_osc_promise
  {
    std::promise<void> promise;
    ossia::net::address_base* address{};
  };
  struct get_ws_promise
  {
    std::promise<void> promise;
    std::string address{};
  };
  using promises_map = locked_map<tsl::hopscotch_map<std::string, get_osc_promise>>;

  moodycamel::ReaderWriterQueue<get_ws_promise> m_getWSPromises;
  moodycamel::ReaderWriterQueue<std::function<void()>> m_functionQueue;
  std::function<void()> m_commandCallback;
  promises_map m_getOSCPromises;

  std::thread m_wsThread;
  std::string m_websocketHost;
};

}
}