//! # Nvhoist
//!

#![warn(clippy::all)]
#![deny(missing_docs)]

use std::env;
use std::ffi::{OsStr, OsString};
use std::os::unix::process::CommandExt;
use std::path::PathBuf;
use std::process::{exit, Command};
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

            if args.len() > 1 {
                let mut path = PathBuf::from(&args[1]);
                if path.is_relative() {
                    path = env::current_dir()
                        .expect("could not get current directory")
                        .join(path);
                }

                // TODO error handling
                let canon = path.parent().unwrap().canonicalize().unwrap();
                match path.file_name() {
                    Some(name) => path = canon.join(name),
                    None => path = canon,
                }

                rpc.notify(format!("split {}", path.to_str().unwrap()).as_str())
                    .expect("failed sending notification to NeoVim");
            }
        }
    }
}
