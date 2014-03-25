proxyhack
=========

An experimentation to make enterprise proxy transparent on linux

If you have been working behind an enterprise proxy which is not transparent,
you know how painful is it.

This project is an experimentation whose goal is to: 
  - make proxy transparent
  - be avaiable for all software running on the system
  - allow user specific configuration

#use

git clone https://github.com/maxired/proxyhack.git # probably you need to setup your proxy first :) and then unset it
cd proxyhack
make
export LD_PRELOAD=`pwd`/proxyhack.so
export PROXY_ADDR=<your_proxy_port>
export PROXY_PORT=<your_proxy_addr>
curl curlmyip.com


#Implementations choices
We decided to override linux C standard library in order to be accessible for any software on the system. We have been massively influenced by [bindhack](daniel-lange.com/software/bindhack.c) and reuse same mechanismes. Another solution would have been to create a virtual network devices, with tun/tap mechanismes.

#Features
proxyhack is currently working with http request along and HTTP proxy. We need to support other protocols.

#TODO
there is lot to do in order to complete the idea :
 - testing : unitary testing would be a need, to make devepements easier, and show people how good the solution work
 - supporting all protocols : http seems to be working right now, but not SSL based protocol.
 - supporting socks proxy : this would allow to also support UDP
 - Advanced configuration : currently proxy settings si done trhought env variable. One proxy will be used for all connection. We want support advanced configuration. I am in favor of Javascript files to do so, as it is format of proxy autoconfiguration file [pac](http://en.wikipedia.org/wiki/Proxy_auto-config) 


