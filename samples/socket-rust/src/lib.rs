use std::io;
use std::io::{Read, Write};
pub use std::net::IpAddr;
use std::net::{Ipv4Addr, Shutdown, SocketAddr};
use std::os::raw::c_char;
use std::str;
mod wasi_poll;
use crate::wasi_poll as poll;

#[derive(Copy, Clone, Debug)]
#[repr(C)]
pub enum AddressFamily {
    Inet4 = 0,
    Inet6 = 1,
}

impl From<&SocketAddr> for AddressFamily {
    fn from(addr: &SocketAddr) -> Self {
        match addr {
            SocketAddr::V4(_) => AddressFamily::Inet4,
            SocketAddr::V6(_) => AddressFamily::Inet6,
        }
    }
}

#[derive(Copy, Clone, Debug)]
#[repr(C)]
pub enum SocketType {
    Dgram = 0,
    Stream = 1,
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct WasiAddressIPV4 {
    pub n0: u8,
    pub n1: u8,
    pub n2: u8,
    pub n3: u8,
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct WasiAddressIPV4Port {
    pub addr: WasiAddressIPV4,
    pub port: u16,
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct WasiAddress {
    pub kind: AddressFamily,
    pub addr: WasiAddressIPV4Port,
}

macro_rules! syscall {
    ($fn: ident ( $($arg: expr),* $(,)* ) ) => {{
        #[allow(unused_unsafe)]
        let res = unsafe { libc::$fn($($arg, )*) };
        if res == -1 {
            Err(std::io::Error::last_os_error())
        } else {
            Ok(res)
        }
    }};
}

fn fcntl_add(fd: u32, get_cmd: i32, set_cmd: i32, flag: i32) -> io::Result<()> {
    let previous = syscall!(fcntl(fd as i32, get_cmd))?;
    let new = previous | flag;
    if new != previous {
        syscall!(fcntl(fd as i32, set_cmd, new)).map(|_| ())
    } else {
        // Flag was already set.
        Ok(())
    }
}

/// Remove `flag` to the current set flags of `F_GETFD`.
fn fcntl_remove(fd: u32, get_cmd: i32, set_cmd: i32, flag: i32) -> io::Result<()> {
    let previous = syscall!(fcntl(fd as i32, get_cmd))?;
    let new = previous & !flag;
    if new != previous {
        syscall!(fcntl(fd as i32, set_cmd, new)).map(|_| ())
    } else {
        // Flag was already set.
        Ok(())
    }
}

impl WasiAddress {
    pub fn from_socket_addr(addr: &SocketAddr) -> WasiAddress {
        let port = addr.port();
        match addr {
            SocketAddr::V4(ipv4) => {
                let b = ipv4.ip().octets();
                WasiAddress {
                    kind: AddressFamily::Inet4,
                    addr: WasiAddressIPV4Port {
                        addr: WasiAddressIPV4 {
                            n0: b[0],
                            n1: b[1],
                            n2: b[2],
                            n3: b[3],
                        },
                        port: port,
                    },
                }
            }
            SocketAddr::V6(_) => panic!("not supported"),
        }
    }
}

#[repr(C)]
pub struct IovecWrite {
    pub buf: *const u8,
    pub size: usize,
}

#[repr(C)]
pub struct IovecRead {
    pub buf: *mut u8,
    pub size: usize,
}

#[derive(Debug)]
pub struct Socket {
    sock_fd: u32,
}

impl Socket {
    pub fn new(addr_family: AddressFamily, sock_kind: SocketType) -> io::Result<Self> {
        unsafe {
            let mut fd = 0;
            let res = sock_open(-1, addr_family as u8, sock_kind as u8, &mut fd);
            if res == 0 {
                Ok(Socket { sock_fd: fd })
            } else {
                Err(io::Error::from_raw_os_error(res as i32))
            }
        }
    }

    pub fn connect(&self, addrs: &SocketAddr) -> io::Result<()> {
        let mut addr = WasiAddress::from_socket_addr(&addrs);
        unsafe {
            let res = sock_connect(self.sock_fd, &mut addr);
            if res != 0 {
                Err(io::Error::from_raw_os_error(res as i32))
            } else {
                Ok(())
            }
        }
    }

    pub fn send(&self, buf: &[u8]) -> io::Result<usize> {
        unsafe {
            let mut send_len: u32 = 0;
            let vec = IovecWrite {
                buf: buf.as_ptr(),
                size: buf.len(),
            };
            let res = sock_send(self.sock_fd, &vec, 1, 0, &mut send_len);
            if res == 0 {
                Ok(send_len as usize)
            } else {
                Err(io::Error::from_raw_os_error(res as i32))
            }
        }
    }

    pub fn recv(&self, buf: &mut [u8]) -> io::Result<usize> {
        let flags = 0;
        let mut recv_len: usize = 0;
        let mut oflags: usize = 0;
        let mut vec = IovecRead {
            buf: buf.as_mut_ptr(),
            size: buf.len(),
        };

        unsafe {
            let res = sock_recv(self.sock_fd, &mut vec, 1, flags, &mut recv_len, &mut oflags);
            if res == 0 {
                Ok(recv_len)
            } else {
                Err(io::Error::from_raw_os_error(res as i32))
            }
        }
    }

    pub fn listen(&self, backlog: i32) -> io::Result<()> {
        unsafe {
            let res = sock_listen(self.sock_fd, backlog as u32);
            if res != 0 {
                Err(io::Error::from_raw_os_error(res as i32))
            } else {
                Ok(())
            }
        }
    }


    pub fn set_nonblocking(&self, nonblocking: bool) -> io::Result<()> {
        if nonblocking {
            fcntl_add(self.sock_fd, libc::F_GETFL, libc::F_SETFL, libc::O_NONBLOCK)
        } else {
            fcntl_remove(self.sock_fd, libc::F_GETFL, libc::F_SETFL, libc::O_NONBLOCK)
        }
    }

    pub fn accept(&self, nonblocking: bool) -> io::Result<Self> {
        unsafe {
            let mut fd: u32 = 0;
            let res = sock_accept(self.sock_fd as u32, &mut fd);
            if res != 0 {
                Err(io::Error::from_raw_os_error(res as i32))
            } else {
                let s = Socket { sock_fd: fd };
                s.set_nonblocking(nonblocking)?;
                Ok(s)
            }
        }
    }
    pub fn bind(&self, addrs: &SocketAddr) -> io::Result<()> {
        unsafe {
            let mut addr = WasiAddress::from_socket_addr(&addrs);
            let res = sock_bind(self.sock_fd, &mut addr);
            if res != 0 {
                Err(io::Error::from_raw_os_error(res as i32))
            } else {
                Ok(())
            }
        }
    }

    pub fn shutdown(&self, how: Shutdown) -> io::Result<()> {
        unsafe {
            let flags = match how {
                Shutdown::Read => 1,
                Shutdown::Write => 2,
                Shutdown::Both => 3,
            };
            let res = sock_shutdown(self.sock_fd as u32, flags);
            if res == 0 {
                Ok(())
            } else {
                Err(io::Error::from_raw_os_error(res as i32))
            }
        }
    }
}

#[derive(Debug)]
pub struct TcpStream {
    s: Socket,
}

impl TcpStream {
    pub fn new(s: Socket) -> Self {
        Self { s }
    }

    pub fn get_fd(&self) -> u32 {
        self.s.sock_fd
    }

    pub fn connect(addr: &SocketAddr) -> io::Result<TcpStream> {
        let s = Socket::new(AddressFamily::from(addr), SocketType::Stream)?;
        s.connect(addr)?;
        Ok(TcpStream { s })
    }

    pub fn shutdown(&self, how: Shutdown) -> io::Result<()> {
        self.s.shutdown(how)
    }
}

impl Read for TcpStream {
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        self.s.recv(buf)
    }
}

impl Write for TcpStream {
    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        self.s.send(buf)
    }
    fn flush(&mut self) -> io::Result<()> {
        Ok(())
    }
}

#[link(wasm_import_module = "wasi_snapshot_preview1")]
extern "C" {
    pub fn sock_open(poll_fd: i32, addr_family: u8, sock_type: u8, fd: *mut u32) -> u32;
    pub fn sock_connect(fd: u32, addr: *mut WasiAddress) -> u32;
    pub fn sock_shutdown(fd: u32, flags: u8) -> u32;

    pub fn sock_listen(fd: u32, backlog: u32) -> u32;
    pub fn sock_accept(fd: u32, fd: *mut u32) -> u32;

    pub fn sock_bind(fd: u32, addr: *mut WasiAddress) -> u32;

    pub fn sock_recv(
        fd: u32,
        buf: *mut IovecRead,
        buf_len: usize,
        flags: u16,
        recv_len: *mut usize,
        oflags: *mut usize,
    ) -> u32;
    pub fn sock_send(
        fd: u32,
        buf: *const IovecWrite,
        buf_len: u32,
        flags: u16,
        send_len: *mut u32,
    ) -> u32;

    pub fn sock_recv_from(
        fd: u32,
        buf: *mut IovecRead,
        buf_len: u32,
        addr: *mut u8,
        flags: u16,
        recv_len: *mut usize,
        oflags: *mut usize,
    ) -> u32;

    pub fn sock_send_to(
        fd: u32,
        buf: *const IovecWrite,
        buf_len: u32,
        addr: *const u8,
        port: u16,
        flags: u16,
        send_len: *mut u32,
    ) -> u32;
}

fn localhost() -> IpAddr {
    IpAddr::V4(Ipv4Addr::new(127, 0, 0, 1))
}

fn simple_http_client() -> io::Result<()> {
    let mut stream = TcpStream::connect(&SocketAddr::new(localhost(), 9000))?;

    stream.write_all("GET /test.html HTTP/1.1\r\n\r\n".as_bytes())?;
    let mut buf = [0; 128];

    loop {
        let size = stream.read(&mut buf)?;
        if size == 0 {
            break;
        }
        println!("response: \n{}", str::from_utf8(&buf).unwrap());
    }

    stream.shutdown(Shutdown::Both)?;

    Ok(())
}

fn poll_http_client() -> io::Result<()> {
    let mut streams = vec![
        TcpStream::connect(&SocketAddr::new(localhost(), 9000))?,
        TcpStream::connect(&SocketAddr::new(localhost(), 9000))?,
    ];

    let mut idx = 0;
    let mut subscriptions: Vec<poll::Subscription> = vec![];
    for stream in &mut streams {
        stream.write_all("GET /test.html HTTP/1.1\r\n\r\n".as_bytes())?;
        subscriptions.push(poll::Subscription {
            userdata: idx,
            u: poll::SubscriptionU {
                tag: poll::EVENTTYPE_FD_READ,
                u: poll::SubscriptionUU {
                    fd_read: poll::SubscriptionFdReadwrite {
                        file_descriptor: stream.get_fd(),
                    },
                },
            },
        });
        idx += 1;
    }

    let mut total_reads = 0;
    let mut run_loop = true;
    while run_loop {
        let mut revent = vec![poll::Event::empty(); subscriptions.len()];
        unsafe {
            let n = poll::poll(
                subscriptions.as_ptr(),
                revent.as_mut_ptr(),
                subscriptions.len(),
            )?;

            for event in revent {
                match event.type_ {
                    poll::EVENTTYPE_FD_READ => {
                        let idx = event.userdata as usize;
                        let mut buf = String::new();
                        streams[idx].read_to_string(&mut buf)?;
                        println!("Data from {}: \n{}", idx, buf);
                        total_reads += 1;
                        if total_reads == subscriptions.len() {
                            run_loop = false;
                        }
                    }
                    poll::EVENTTYPE_CLOCK => {
                        println!("CLOCK")
                    }
                    _ => unreachable!(),
                }
                println!("{}", event.type_);
            }
        }
    }

    for stream in &mut streams {
        stream.shutdown(Shutdown::Both)?;
    }
    Ok(())
}

fn simple_tcp_echo_server() -> io::Result<()> {
    let addr = &SocketAddr::new(localhost(), 1234);
    let s = Socket::new(AddressFamily::from(addr), SocketType::Stream)?;
    s.bind(&addr)?;

    s.listen(3)?;

    let new_socket = s.accept(true)?;

    let data = new_socket.send("hello\n".as_bytes())?;
    println!("Data: {data}");

    let mut buf = [0; 128];
    new_socket.recv(&mut buf)?;

    Ok(())
}

fn run_example(f: fn() -> io::Result<()>, name: &str) {
    println!("============== Executing {} ==============", name);
    let res = f();
    if res.is_err() {
        println!("Error: {:?}", res.err());
    }
    println!("============== Completed {} ==============", name);
}

#[no_mangle]
pub extern "C" fn main(_argv: *const *const c_char, _argc: i32) -> i32 {
    run_example(poll_http_client, "poll http client");
    run_example(simple_http_client, "simple http client");
    run_example(simple_tcp_echo_server, "simple tcp echo server");

    0
}
