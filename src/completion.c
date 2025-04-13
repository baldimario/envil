#include <stdio.h>
#include <string.h>
#include "completion.h"
#include "config.h"
#include "checks.h"

ShellType get_shell_type(const char* shell_name) {
    if (!shell_name) return SHELL_UNKNOWN;
    
    if (strcasecmp(shell_name, "bash") == 0) return SHELL_BASH;
    if (strcasecmp(shell_name, "zsh") == 0) return SHELL_ZSH;
    
    return SHELL_UNKNOWN;
}

static void write_bash_completion(FILE* out) {
    fprintf(out, "_envil() {\n");
    fprintf(out, "    local cur prev words cword split\n");
    fprintf(out, "    _init_completion -s || return\n\n");
    
    // Handle options that take file arguments
    fprintf(out, "    case $prev in\n");
    fprintf(out, "        -c|--config)\n");
    fprintf(out, "            _filedir '@(yml|yaml|json)'\n");
    fprintf(out, "            return\n");
    fprintf(out, "            ;;\n");
    fprintf(out, "        -e|--env)\n");
    fprintf(out, "            COMPREPLY=($(compgen -e -- \"${cur}\"))\n");
    fprintf(out, "            return\n");
    fprintf(out, "            ;;\n");
    fprintf(out, "        --type)\n");
    fprintf(out, "            COMPREPLY=($(compgen -W \"string integer float json\" -- \"${cur}\"))\n");
    fprintf(out, "            return\n");
    fprintf(out, "            ;;\n");
    fprintf(out, "    esac\n\n");

    // Handle general options
    fprintf(out, "    if [[ $cur == -* ]]; then\n");
    fprintf(out, "        COMPREPLY=($(compgen -W '");
    
    // Add base options
    for (size_t i = 0; i < get_base_options_count(); i++) {
        if (base_options[i].name) {
            fprintf(out, "--%s ", base_options[i].name);
            if (base_options[i].val) {
                fprintf(out, "-%c ", (char)base_options[i].val);
            }
        }
    }
    
    // Add check options
    for (size_t i = 0; i < get_check_options_count(); i++) {
        if (check_options[i].name) {
            fprintf(out, "--%s ", check_options[i].name);
        }
    }
    
    fprintf(out, "' -- \"${cur}\"))\n");
    fprintf(out, "        return\n");
    fprintf(out, "    fi\n");
    fprintf(out, "}\n\n");
    fprintf(out, "complete -F _envil envil\n");
}

static void write_zsh_completion(FILE* out) {
    fprintf(out, "#compdef envil\n\n");
    fprintf(out, "_envil() {\n");
    fprintf(out, "    local -a options\n");
    fprintf(out, "    local -a check_options\n\n");
    
    // Define base options
    fprintf(out, "    options=(\n");
    for (size_t i = 0; i < get_base_options_count(); i++) {
        if (base_options[i].name) {
            if (base_options[i].has_arg == required_argument) {
                fprintf(out, "        '(--%s -%c)'{--%s,-%c}'[%s]:value:_files'\n",
                    base_options[i].name,
                    (char)base_options[i].val,
                    base_options[i].name,
                    (char)base_options[i].val,
                    base_options[i].name);
            } else {
                fprintf(out, "        '(--%s -%c)'{--%s,-%c}'[%s]'\n",
                    base_options[i].name,
                    (char)base_options[i].val,
                    base_options[i].name,
                    (char)base_options[i].val,
                    base_options[i].name);
            }
        }
    }
    fprintf(out, "    )\n\n");
    
    // Define check options
    fprintf(out, "    check_options=(\n");
    for (size_t i = 0; i < get_check_options_count(); i++) {
        if (check_options[i].name) {
            fprintf(out, "        '--%s[%s]:value:'\n",
                check_options[i].name,
                checks[i].description);
        }
    }
    fprintf(out, "    )\n\n");
    
    // Special case completions
    fprintf(out, "    case $words[CURRENT-1] in\n");
    fprintf(out, "        --type)\n");
    fprintf(out, "            _values 'types' string integer float json\n");
    fprintf(out, "            return\n");
    fprintf(out, "            ;;\n");
    fprintf(out, "        -c|--config)\n");
    fprintf(out, "            _files -g '*.{yml,yaml,json}'\n");
    fprintf(out, "            return\n");
    fprintf(out, "            ;;\n");
    fprintf(out, "        -e|--env)\n");
    fprintf(out, "            _parameters -g \"*\"\n");
    fprintf(out, "            return\n");
    fprintf(out, "            ;;\n");
    fprintf(out, "    esac\n\n");
    
    // Main completion
    fprintf(out, "    _arguments -s -S \\\n");
    fprintf(out, "        $options \\\n");
    fprintf(out, "        $check_options\n");
    fprintf(out, "}\n\n");
    fprintf(out, "_envil \"$@\"\n");
}

int generate_completion_script(ShellType shell, FILE* output) {
    if (!output) return -1;
    
    switch (shell) {
        case SHELL_BASH:
            write_bash_completion(output);
            break;
        case SHELL_ZSH:
            write_zsh_completion(output);
            break;
        default:
            fprintf(stderr, "Unsupported shell type\n");
            return -1;
    }
    
    return 0;
}
