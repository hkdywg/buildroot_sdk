#!/bin/bash
# Script to install bash completion for build.sh
# Usage: 
#   source install-completion.sh  (load for current session)
#   ./install-completion.sh       (add to ~/.bashrc for permanent use)

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
COMPLETION_FILE="${SCRIPT_DIR}/build.sh-completion.bash"
BASHRC_FILE="${HOME}/.bashrc"

if [ ! -f "$COMPLETION_FILE" ]; then
    echo "Error: Completion file not found: $COMPLETION_FILE" >&2
    exit 1
fi

# Source the completion file for current session
if [ -f "$COMPLETION_FILE" ]; then
    source "$COMPLETION_FILE" 2>/dev/null
    if [ $? -eq 0 ]; then
        echo "✓ Bash completion for build.sh has been loaded for this session."
        echo "  Try: ./build.sh <TAB> to see available projects"
        echo "  Try: ./build.sh rcar_e3_control_box <TAB> to see available targets"
    fi
fi

# If executed (not sourced), add to bashrc
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    if [ -f "$BASHRC_FILE" ]; then
        if ! grep -q "build.sh-completion.bash" "$BASHRC_FILE" 2>/dev/null; then
            echo "" >> "$BASHRC_FILE"
            echo "# Build.sh TAB completion" >> "$BASHRC_FILE"
            echo "source ${COMPLETION_FILE} 2>/dev/null" >> "$BASHRC_FILE"
            echo "✓ Bash completion for build.sh has been added to ~/.bashrc"
            echo "  Run 'source ~/.bashrc' or restart your terminal to enable TAB completion permanently"
        else
            echo "✓ Bash completion for build.sh is already configured in ~/.bashrc"
        fi
    fi
fi
