#include "Connector.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Callbacks.h"

#include <stdio.h>

EventLoop* g_loop;

void connectCallback(int sockfd)
{
  printf("connected.\n");
  g_loop->quit();
}

int main(int argc, char* argv[])
{
  EventLoop loop;
  g_loop = &loop;
  InetAddress addr(9981, "127.0.0.1");
  // ConnectorPtr connector(new Connector(&loop, addr));
  // connector->setNewConnectionCallback(connectCallback);
  // connector->start();

  // loop.loop();
}