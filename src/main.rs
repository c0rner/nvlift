//! # Nvhoist
//!

#![warn(clippy::all)]
#![deny(missing_docs)]

use std::ffi::{OsStr, OsString};
use std::os::unix::process::CommandExt;
use std::process::exit;
use std::{env, process::Command};
mod nvim_rpc;

/// Name of NVim env variable containing RPC socket path
const NVIM_ENV_LISTENADDR: &str = "NVIM_LISTEN_ADDRESS";
/// List of well-known executable names for NVim
const NVIM_EXECUTABLES: &[&str] = &["nvim", "nvim.appimage"];

/// Execute the binary at `path` using `args` for arguments
fn exec(path: &OsStr, args: &[OsString]) -> impl std::error::Error {
    let mut cmd = Command::new(path);
    cmd.args(args);
    cmd.exec()
}

fn main() {
    let args: Vec<OsString> = env::args_os().collect();

    match env::var_os(NVIM_ENV_LISTENADDR) {
        None => {
            // Environment not set, run in wrapper mode
            if args.len() > 2 && args[1].eq("-e") {
                let e = exec(&args[2], &args[3..]);
                println!("error: {}", e);
                return;
            } else {
                for target in NVIM_EXECUTABLES.iter() {
                    exec(OsString::from(&target).as_os_str(), &args[1..]);
                }
            }
            // Getting here means we have failed launching a target executable,
            // terminate and return non-zero exit code
            exit(1);
        }
        Some(sockname) => {
            let mut rpc = match nvim_rpc::connect(&sockname) {
                Ok(rpc) => rpc,
                Err(e) => {
                    println!("failed connecting to rpc socket {:?} ({})", sockname, e);
                    exit(1);
                }
            };
            // TODO query CWD from nvim process to correctly provide canonical path

            rpc.notify(format!("split {}", args[1].to_str().unwrap()).as_str())
                .expect("failed sending notification to NeoVim");
        }
    }
}
