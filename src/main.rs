//! # Nvhoist
//!

#![warn(clippy::all)]
#![deny(missing_docs)]

//use getopts::Options;
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

    println!("args: {:?}", args);

    match env::var_os(NVIM_ENV_LISTENADDR) {
        None => {
            // Not a nested NeoVim session, try wrapping target executable
            if args.len() > 2 && args[1].eq("-e") {
                exec(&args[2], &args[3..]);
                return;
            } else {
                // No target given, walk the well-known list of executables
                for target in NVIM_EXECUTABLES.iter() {
                    exec(OsString::from(&target).as_os_str(), &args[1..]);
                }
            }
            // Getting here means we have exhausted all possible options.
            // Terminate and return non-zero exit code
            exit(1);
        }
        Some(sockname) => {
            println!("Nested!");
            let rpc = match nvim_rpc::connect(&sockname) {
                Ok(rpc) => rpc,
                Err(e) => {
                    println!("Connect failed {:?} ({})", sockname, e);
                    exit(1);
                }
            };
            rpc.notify(format!("split {}", args[1].to_str().unwrap()).as_str())
                .expect("failed sending notification to NeoVim");
        }
    }
}
