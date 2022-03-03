#!/bin/bash

VM_ROOT="/workspace"

cd ..

proj_root_escaped="$(echo "$(pwd)" | sed -e 's/\//\\\//g')"
vm_root_escaped="$(echo "$VM_ROOT" | sed -e 's/\//\\\//g')"

rm -f "compile_commands.json"
vagrant ssh --command "cd $VM_ROOT && make clean-all && bear make"
sed -i -e "s/$vm_root_escaped/$proj_root_escaped/g" "compile_commands.json"