#!/bin/bash
# Bash completion script for build.sh
# Usage: source this file or add to ~/.bash_completion.d/

_build_sh_completion() {
    local cur prev words cword
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    words=("${COMP_WORDS[@]}")
    cword=$COMP_CWORD

    # Get the script directory
    # Try multiple methods to find build.sh location
    local script_dir
    local script_path="${COMP_WORDS[0]}"
    
    # Normalize script path (handle ./build.sh, build.sh, absolute paths)
    if [[ "$script_path" == ./* ]]; then
        # Relative path like ./build.sh
        script_path="${PWD}/${script_path#./}"
    elif [[ "$script_path" != /* ]]; then
        # Relative path without ./, try to resolve
        if [ -f "./${script_path}" ]; then
            script_path="${PWD}/${script_path}"
            echo "2"
        elif command -v "$script_path" >/dev/null 2>&1; then
            # If it's in PATH, get its location
            script_path=$(command -v "$script_path")
            echo "3"
        fi
        echo "4"
    fi

    # Method 1: If script path exists and is a file 
    if [ -f "$script_path" ]; then 
        script_dir=$(cd -- "$(dirname -- "$script_path")" &> /dev/null && pwd) 
    # Method 2: Try current directory 
    elif [ -f "./build.sh" ]; then 
        script_dir=$(pwd) 
    # Method 3: Try relative to completion script location 
    elif [ -n "${BASH_SOURCE[0]}" ] && [ -f "${BASH_SOURCE[0]%/*}/build.sh" ]; then 
        script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd) 
    # Method 4: Last resort - use current directory 
    else 
        script_dir=$(pwd) 
    fi
    
    #script_dir=$(cd -- "$(dirname -- "${script_path}")" && pwd)

    # Get available projects from device directory (same logic as build.sh)
    local projects
    if [ -d "${script_dir}/device" ]; then
        projects=$(find "${script_dir}/device" -mindepth 1 -maxdepth 1 -not -path "*/common" -type d -print 2>/dev/null | awk -F/ '{print $NF}' | sort -t '-' -k2,2)
    fi

    # Supported subtargets (must match build.sh)
    local subtargets="uboot kernel kernel_dtb kernel_driver rootfs boot gpt system"
    
    # First argument: project name or "lunch"
    if [ $cword -eq 1 ]; then
        #echo "curent scripts dir is : ${script_dir} --"
        local options="lunch"
        if [ -n "$projects" ]; then
            options="$options $projects"
        fi
        COMPREPLY=($(compgen -W "$options" -- "$cur"))
        return 0
    fi

    # Second argument: subtarget (only if first arg is a project, not "lunch")
    if [ $cword -eq 2 ]; then
        local first_arg="${words[1]}"
        # Check if first argument is a valid project (not "lunch")
        if [ "$first_arg" != "lunch" ] && [ -n "$projects" ]; then
            local is_project=0
            for proj in $projects; do
                if [ "$proj" = "$first_arg" ]; then
                    is_project=1
                    break
                fi
            done
            if [ $is_project -eq 1 ]; then
                COMPREPLY=($(compgen -W "$subtargets" -- "$cur"))
                return 0
            fi
        fi
    fi

    return 0
}

# Register completion function for various ways to invoke build.sh
# This will work for: ./build.sh, build.sh, bash build.sh, etc.
complete -F _build_sh_completion build.sh
complete -F _build_sh_completion ./build.sh
