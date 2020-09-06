# Nvhoist
Nvhoist is a minimalistic Nvim wrapper designed to hoist/move nested nvim session from
within a `terminal` window up into the top level session.

When using `nvim` as a terminal multiplexer this is a common issue and can be of great
annoyance where nested sessions break things like LSP plugins.

## Installation
It is recommended to install `nvhoist` as an alias to what you normally start Nvim with.
Aliasing should be set up so that it is done for any new interactive shell at startup.

#### Bash example
This example assumes there already is an alias`vim` for `nvim` and that the target is the
full executable path.

In `~/.bashrc` append the following:
``` bash
# Re-configure alias only if we are inside a running Nvim
if [ -n $NVIM_LISTEN_ADDRESS ]; then
  alias vim="nvhoist ${BASH_ALIASES[vim]}"
fi
```

## Usage
N/A

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License
[MIT](https://choosealicense.com/licenses/mit/)
