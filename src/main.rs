//! # Nvhoist
//!

#![warn(clippy::all)]
#![deny(missing_docs)]

use argh::FromArgs;
use std::ffi::{OsStr, OsString};
use std::fs;
use std::io::Error;
use std::os::unix::{fs::FileTypeExt, process::CommandExt};
use std::process::exit;
use std::{env, process::Command};

/// Environment variable containing full path to NVim socket
const NVIM_ENV_LISTENADDR: &str = "NVIM_LISTEN_ADDRESS";
/// Array with well-known executable names for NVim that will
/// be used for automatic target discovery.
const NVIM_EXECUTABLES: &[&str] = &["nvim", "nvim.appimage"];

#[derive(FromArgs)]
/// Are we nested yet?
struct Options {
    #[argh(positional)]
    args: Vec<OsString>,
    #[argh(switch)]
    /// enable debug output
    debug: bool,
    #[argh(option, short = 't')]
    /// wrapped executable
    target: Option<OsString>,
}

/// Execute the binary designeted by `path` with `args` as arguments
fn exec(path: &OsStr, args: &[OsString]) -> Error {
    let mut cmd = Command::new(path);
    cmd.args(args);
    cmd.exec()
}

/// Test if the file at `path` is a unix socket file
fn is_socket(path: &OsStr) -> Result<bool, Error> {
    let meta = fs::metadata(path)?;
    Ok(meta.file_type().is_socket())
}

fn main() {
    let cfg: Options = argh::from_env();
    let named_sock = env::var_os(NVIM_ENV_LISTENADDR);

    match named_sock {
        Some(ns) => {
            println!("Lets do: {:?}", &ns.to_str());
            match is_socket(ns.as_os_str()) {
                Ok(socket) => {
                    println!("Cool, we have socket");
                }
                Err(e) => {
                    println!("socket connect failed: {}", e);
                    exit(1);
                }
            };
        }
        None => {
            // Not a nested session, try wrapper mode
            match cfg.target {
                Some(target) => {
                    exec(&target, &cfg.args);
                    return;
                }
                None => {
                    // No target given, check the list of executables
                    for target in NVIM_EXECUTABLES.iter() {
                        println!("Guessing executable: {}", &target);
                        exec(OsString::from(&target).as_os_str(), &cfg.args);
                    }
                    return;
                }
            }
        }
    }
}
