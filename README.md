# Nvlift
Nvlift is a minimalistic wrapper for Neovim used to move nested Nvim session from
within a `:terminal` window up into the top level Nvim session.

When using `nvim` as a terminal multiplexer this will be a regular annoyance and usually
also break things like `lsp` plugins.

## Installation
There are two distinct modes `nvlift` can be run in, "full wrapper" or "nested wrapper".
They are both fairly simple to configure however nested mode is the recommended only
becasue it has a better defined scope.

### Full wrapper
In this setup `nvlift` will alias the `nvim` command with the '-e' switch argument set to
the nvim binary name or path.  If not using a full path the file name will be searched
for in the `PATH` environment variable.

##### Example (Bash) 
``` bash
# ~/.bashrc
# Re-alias `nvim` and `vim`
alias nvim="nvlift -e /path/to/nvim"
alias vim="nvim"
```

### Nested wrapper
This setup has the smallest footprint and is preferred over a full wrapper. It requires
that the re-aliasing is done for every new interactive shell. For `bash` this is normally
the `~/.bashrc` file.

#### Example (Bash)
``` bash
# ~/.bashrc
# Re-configure alias only if we are running inside a Nvim session
if [ -n $NVIM_LISTEN_ADDRESS ]; then
  alias vim="nvlift"
fi
```

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License
[MIT](https://choosealicense.com/licenses/mit/)
