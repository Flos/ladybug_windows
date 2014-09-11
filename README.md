ladybug_network-windows_client
==============================
Windows applications to 
  captures images form the Ladybug5 and to stream then with zmq and protobuf to a ros node. (ladybug_ros: git@github.com:Flos/ladybug_ros.git)
  extract the rectification maps for each lens.
  
 Requires:
  -Protobuf:     https://code.google.com/p/protobuf/
  -Ladybug SDK:  http://www.ptgrey.com/support/downloads/downloads_admin/index.aspx
  -ZMQ 3.2.x:    http://zeromq.org/intro:get-the-software
  -libjpg-Turbo: http://libjpeg-turbo.virtualgl.org/
  -Boost:        http://www.boost.org/
  -OpenCV2:      http://opencv.org/

 Requirements and add the following system variables to your installation paths 
  PROTOBUF:  C:\opt\protobuf
  BOOST:     C:\opt\boost_1_55_0
  ZMQ:       C:\opt\zeromq
  LADYBUG:   C:\Program Files\Point Grey Research\Ladybug
  JPG_TURBO: C:\opt\libjpeg-turbo64
  OPENCV:    C:\opt\opencv
  path:      C:\opt\protobuf\vsprojects\x64\Release; 

Installation:
  git clone --recursiv git@github.com:Flos/ladybug_network-windows_client.git  
  cd ladybug_network-windows_client/protobuf
  protoc imageMessage.proto --cpp_out=.
   
  Open Ladybug5_Network.sln with Visual Studio 2012

  