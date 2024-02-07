#!/bin/bash

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Rene Wolf

# the alsa device we expect the clock gen to be
alsadevice=hw:CARD=CXADCADCClockGe

tmp=/var/tmp/info
# create a tmp dir to collect data in
mkdir -p $tmp


function debug_mute_switch
{
	amixer -D $alsadevice cset name='Audio Control Capture Switch' $1
}

function short_record
{
	local start=$(date +%s)
	arecord -D $alsadevice -c 3 -r 46875 -f S24_3LE --samples=1000 "$1"
	local e=$?
	local end=$(date +%s)
	echo "exit $e, time $(( $end - $start )) sec" > "${1}.nfo"
}

# try a 1000 sample test capture on actual adc data, to see what happens
debug_mute_switch on
short_record $tmp/test.wav

# switch to debug output and get 1000
debug_mute_switch off
short_record $tmp/debug.wav

# switch back to adc data
debug_mute_switch on

# The next few commands collect some general system information that needs elevated privileges
# They mostly aim to identify troubles with usb connections or other such communication problems
sudo dmesg > "$tmp/dmesg.txt"
sudo cat /var/log/syslog > "$tmp/syslog"
sudo cat /var/log/kern.log > "$tmp/kern.log"
# This lsusb gives us the usb device details including resolved string descriptors.
# It will contain the build version of the clock gen firmware and also give some insight into some cases of usb troubles
sudo lsusb -v > "$tmp/lsusb"

# Now we check if arecord can see the device, once as root, and once without
# If root works but normal user don't then this may be a permissions problem
sudo arecord -L > "$tmp/arecord-devices-sudo"
arecord -L > "$tmp/arecord-devices-nosudo"

# The remaining commands collect some more info that do not require elevated privileges
lsusb -t > "$tmp/lsusb-tree" # a nice tree view of your usb
uname -a > "$tmp/uname" # what kernel / arch you are running
lsb_release -a > "$tmp/lsb-release" # if on something debian based this tells us your distro version
arecord --version > "$tmp/arecord-version" # check the version of arecord
groups > "$tmp/groups" # see what groups you are in, this also controls access to sound devices (on some distros)
amixer -D $alsadevice > "$tmp/amixer" # check if amixer can see the device controls 


# this just packs all the files into a tar.gz in your current working dir and removes the temp dir again
tgz="info-$(date +%Y%m%d-%H%M%S).tar.gz"
tar cv $tmp | gzip -9 > "$tgz"
rm -rf "$tmp"

echo "Created $(realpath "$tgz")"
