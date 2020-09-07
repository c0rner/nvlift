# Nvhoist
Nvhoist is a minimalistic Nvim wrapper designed to hoist/move nested nvim session from
within a `terminal` window up into the top level session.

When using `nvim` as a terminal multiplexer this will be a regular annoyance and usually
also break things like `lsp` plugins.

## Installation
There are two distinct modes `nvhoist` can be run in, "full wrapper" or "nested wrapper".
They are both fairly simple to configure however nested mode is the recommended only
becasue it has a better defined scope.

### Full wrapper
In this setup `nvhoist` will alias the `nvim` command with the first argument set to
the nvim binary name or path.  If not using a full path the file name will be searched
for in the `PATH` environment variable.

##### Example (Bash) 
``` bash
# ~/.bashrc
# Re-alias `nvim` and `vim`
alias nvim="nvhoist /path/to/nvim"
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
  alias vim="nvhoist"
fi
```

## Usage
N/A

## Compiling
Compiling `nvhoist` requires that you have a working C99 compiler like `gcc`.
```
gcc -Wall -O2 main.c -o nvhoist && strip nvhoist
```

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License
[MIT](https://choosealicense.com/licenses/mit/)
