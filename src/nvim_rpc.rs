use msgpack_simple::MsgPack;
use std::ffi::OsString;
use std::io::{Read, Result, Write};
use std::os::unix::net::UnixStream;
use std::time::Duration;

pub struct Rpc {
    // RPC socket used to communicate with NVim
    sock: UnixStream,
}

impl Rpc {
    fn new(sock: UnixStream) -> Self {
        Rpc { sock }
    }

    pub fn notify(&mut self, cmd: &str) -> Result<()> {
        let message = MsgPack::Array(vec![
            MsgPack::Int(2), // Message type 2 is for notifications
            MsgPack::String(String::from("nvim_command")),
            MsgPack::Array(vec![MsgPack::String(cmd.to_string())]),
        ]);
        let data = message.encode();
        self.sock.write_all(&data)?;
        let mut buf = Vec::new();
        match self.sock.read(&mut buf) {
            Ok(_) => Ok(()),
            Err(e) => match e.kind() {
                std::io::ErrorKind::WouldBlock => Ok(()),
                _ => Err(e),
            },
        }
    }
}

pub fn connect(path: &OsString) -> Result<Rpc> {
    let sock = UnixStream::connect(path)?;
    sock.set_read_timeout(Some(Duration::new(1, 0)))?;
    sock.set_write_timeout(Some(Duration::new(1, 0)))?;
    Ok(Rpc::new(sock))
}
