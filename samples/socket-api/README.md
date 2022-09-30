# "socket-api" sample introduction

This sample demonstrates how to use WAMR socket-api to develop wasm network applications.
Two wasm applications are provided: tcp-server and tcp-client, and this sample demonstrates
how they communicate with each other.

## Preparation

Please install WASI SDK, download the [wasi-sdk release](https://github.com/CraneStation/wasi-sdk/releases) and extract the archive to default path `/opt/wasi-sdk`.
And install wabt, download the [wabt release](https://github.com/WebAssembly/wabt/releases) and extract the archive to default path `/opt/wabt`

## Build the sample

```bash
mkdir build
cd build
cmake ..
make
```

`iwasm` and three Wasm modules, `tcp_server.wasm`, `tcp_client.wasm`, `send_recv.wasm`
will be generated. And their corresponding native version, `tcp_server`,
`tcp_client`, `send_recv` are generated too.

> Note that iwasm is built with libc-wasi and lib-pthread enabled.

## Run workload

Start the tcp server, which opens port 1234 and waits for clients to connect.

```bash
cd build
./iwasm --addr-pool=0.0.0.0/15 tcp_server.wasm
```

Start the tcp client, which connects the server and receives message.

```bash
cd build
./iwasm --addr-pool=127.0.0.1/15 tcp_client.wasm
```

The output of client is like:

```bash
[Client] Create socket
[Client] Connect socket
[Client] Client receive
[Client] 115 bytes received:
Buffer recieved:
Say Hi from the Server
Say Hi from the Server
Say Hi from the Server
Say Hi from the Server
Say Hi from the Server

[Client] BYE
```

`send_recv.wasm` contains a thread as a server and a thread as a client. They
send and receive data via 127.0.0.1:1234.

```bash
$ ./iwasm --addr-pool=127.0.0.1/0  ./send_recv.wasm
```

The output is:

```bash
Server is online ...
Client is running...
Start receiving.
Start sending.
Send 106 bytes successfully!
Receive 106 bytes successlly!
Data:
  The stars shine down
  It brings us light
  Light comes down
  To make us paths
  It watches us
  And mourns for us
```

`socket_opts.wasm` shows examples of various ways of using socket options. For example, executing socket_opts with `setget` shows the different options that WAMR supports:
```bash
$ ./iwasm ./socket_opts.wasm setget
```

The different examples that are demonstrated in `socket_opts`:
* `setget` - Sets and gets various socket options supported by WAMR
    * `./iwasm socket_opts.wasm setget`
* `timeout_server` & `timeout_client` - Demonstrates a send/recv timeout using the `SO_SNDTIMEO` & `SO_RCVTIMEO` socket options. 
    * `./iwasm --addr-pool=0.0.0.0/15 socket_opts.wasm timeout_server`
    * `./iwasm --addr-pool=127.0.0.1/15 --heap-size=10000000 socket_opts.wasm timeout_client`
* `multicast_client` & `multicast_server` - Demonstrates how to recv multicast datagrams. 
    * `./iwasm --addr-pool=127.0.0.1/15 --heap-size=10000000 socket_opts.wasm multicast_client`
    * `./socket_opts multicast_server` (Only the client is run with WAMR, as sending multicast datagrams is unsupported)
* `multicast_client` & `multicast_server` - Demonstrates how to recv multicast datagrams. 
    * `./iwasm --addr-pool=127.0.0.1/15 --heap-size=10000000 socket_opts.wasm multicast_client_inet6`
    * `./socket_opts multicast_server_inet6` (Only the client is run with WAMR, as sending multicast datagrams is unsupported)

Refer to [socket api document](../../doc/socket_api.md) for more details.
