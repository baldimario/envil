# Envil

A lightweight C-based CLI tool for environment variable validation and management.

<br/>
<p align="center">
    <img src="https://github.com/baldimario/envil/blob/main/doc/envil.png?raw=true" width="300px" height="300px" alt="Envil Logo" />
</p>

## Features

- Validate environment variables through CLI or configuration files (YAML/JSON)
- Multiple validation types:
  - Type checking (string, integer, float, json)
  - Numeric comparisons (gt, lt)
  - String validation (length, enum)
  - Custom validation via shell commands
- Default values support
- Error reporting with descriptive messages
- Exit codes for CI/CD integration

## Dependencies

The project requires the following libraries:

- libyaml: For YAML configuration file parsing
- json-c: For JSON configuration file parsing

### Installing Dependencies

On Debian/Ubuntu systems:
```bash
sudo apt-get install libyaml-dev libjson-c-dev
```

On Red Hat/Fedora systems:
```bash
sudo dnf install libyaml-devel json-c-devel
```

On macOS with Homebrew:
```bash
brew install libyaml json-c
```

## Installation

```bash
make
sudo make install
```

## Usage

### Command Line Interface

Basic variable validation:
```bash
envil -e VAR_NAME [options]
```

Options:
- `-e, --env NAME`: Environment variable to validate
- `-d, --default VALUE`: Default value if not set
- `-p, --print`: Print value if validation passes
- `-c, --config FILE`: Use configuration file
- `-v, --verbose`: Enable verbose logging
- `-l, --list-checks`: List available checks
- `-h, --help`: Show help message

### Examples

Validate enum with verbose output:
```bash
envil -e LOG_LEVEL --enum "debug,info,warn,error" -v
```

Validate integer range:
```bash
envil -e PORT --type integer --gt 1024 --lt 65535 -v
```

Using default value:
```bash
envil -e API_URL -d "http://localhost:8080" --type string -p
```

Using configuration file with verbose output:
```bash
envil -c config.yml -v
```

Setting environment variables with defaults:
```bash
# Set FOO with a default value and enum validation
FOO=$(envil -e FOO -d bar --enum bar,baz,buz)

# Export multiple variables with defaults
export DB_HOST=$(envil -e DB_HOST -d localhost --type string)
export DB_PORT=$(envil -e DB_PORT -d 5432 --type integer --gt 1024 --lt 65535)
export LOG_LEVEL=$(envil -e LOG_LEVEL -d info --enum debug,info,warn,error)
```

#### Type Checking
```bash
# Integer validation
envil -e PORT --type integer --gt 1024 --lt 65535

# Float validation
envil -e THRESHOLD --type float --gt 0 --lt 1

# JSON validation
envil -e CONFIG --type json
```

#### String Validation
```bash
# Enum validation
envil -e LOG_LEVEL --type string --enum "debug,info,warn,error"

# Length validation
envil -e API_KEY --type string --len 32
```

#### Custom Command Validation
```bash
# Using shell command for validation
envil -e GIT_BRANCH --cmd "git rev-parse --verify HEAD"
```

#### Try Example Cases
Run the examples script to see various validation scenarios in action:
```bash
./examples/examples.sh
```

This script demonstrates:
- Handling missing environment variables
- Using default values
- Type validation
- Numeric range validation
- Configuration file validation with both YAML and JSON
- Multiple variable validation at once

### Configuration File

You can specify multiple validations in a YAML or JSON configuration file:

```yaml
PORT:
  checks:
    type: integer
    gt: 1024
    lt: 65535

LOG_LEVEL:
  default: "info"
  checks:
    type: string
    enum: debug,info,warn,error

CONFIG:
  checks:
    type: json
```

Use the configuration with:
```bash
envil -c config.yml
```

## Exit Codes

- 0: All validations passed
- 1: Configuration error (invalid config file or options)
- 2: Missing required variable
- 3: Type validation failed
- 4: Value validation failed (gt, lt, len, enum)
- 5: Custom command check failed

## Development

### Running Tests
```bash
make test
```

### Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Run the tests (`make test`)
5. Commit your changes (`git commit -m 'Add some amazing feature'`)
6. Push to the branch (`git push origin feature/amazing-feature`)
7. Open a Pull Request

## License

MIT License - see [LICENSE](./LICENSE) file for details.