#!/bin/bash

if [ -z "$1" ]; then
  echo "Usage: $0 <core_name>"
  echo "Available core names:"
  ls -l /dev/remoteproc/ | awk '{ print $9 }' | cut -d '/' -f 4 | sed '/^$/d'
  exit 1
fi

core_name="$1"

remote_proc=$(ls -l /dev/remoteproc/ | grep "$core_name" | sed 's/.*remoteproc\(.*\)$/\1/')

if [ -z "$remote_proc" ]; then
  echo "Core not found."
  exit 1
fi

echo "$remote_proc"

