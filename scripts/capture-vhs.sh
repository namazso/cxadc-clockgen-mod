#!/bin/bash

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Rene Wolf

CLOCK_GEN_ALSA_DEVICE=hw:CARD=CXADCADCClockGe

# NOTE should be adapted to your needs untill cli parsing is implemented
CLOCK_GEN_OUT_VIDEO=0
CXCARD_VIDEO_DEVICE=0
CXCARD_VIDEO_LEVEL=0
CXCARD_VIDEO_VMUX=0

# NOTE should be adapted to your needs untill cli parsing is implemented
CLOCK_GEN_OUT_AUDIO=1
CXCARD_AUDIO_DEVICE=1
CXCARD_AUDIO_LEVEL=0
CXCARD_AUDIO_VMUX=0

# https://stackoverflow.com/questions/192319/how-do-i-know-the-script-file-name-in-a-bash-script
MY_NAME=$(basename "$0")

function die
{
	echo "$@" >&2
	exit 1
}

function card_sysfs
{
	echo -n "/sys/class/cxadc/$1/device/parameters"
}

function f_str_to_switch_name
{
	echo "CXADC-$1"
}

function clock_gen_out_number_to_switch_name
{
	echo "CXADC-Clock $1 Select Playback Source,0"
}

function setup_clock_gen
{
	local clock_gen_out_number=$1  # 0 or 1
	local clock_gen_fstr=$2        # frequency of the clock gen as a string: '20MHz' '28.63MHz' '40MHz' '50MHz'

	local switch_name="$(f_str_to_switch_name $clock_gen_fstr)"

	amixer -D $CLOCK_GEN_ALSA_DEVICE sset "$(clock_gen_out_number_to_switch_name $clock_gen_out_number)" "$switch_name" > /dev/null

	# amixer output contains this line
	#   Item0: 'CXADC-28.63MHz'
	local current_selection="$(amixer -D $CLOCK_GEN_ALSA_DEVICE sget "$(clock_gen_out_number_to_switch_name $clock_gen_out_number)" | grep Item0 | cut -d ":" -f 2 | tr -d " '")"
	if [[ "$current_selection" != "$switch_name" ]] ; then
		die "Setting switch $clock_gen_out_number to '$switch_name' failed, current value is '$current_selection'"
	fi
}


function setup_audio_card
{
	local sysfs_dir="$(card_sysfs cxadc${CXCARD_AUDIO_DEVICE})"
	if [[ ! -d "$sysfs_dir" ]] ; then die "Can't find audio cxadc card at '$sysfs_dir'" ; fi

	echo $CXCARD_AUDIO_VMUX   > $sysfs_dir/vmux
	echo 0                    > $sysfs_dir/sixdb   # 0 off 1 +6db
	echo $CXCARD_AUDIO_LEVEL  > $sysfs_dir/level   # 0 min gain ... 31 max gain 
	echo 0                    > $sysfs_dir/tenxfsc # 0=1.0  1=1.24  2=1.4
	echo 0                    > $sysfs_dir/tenbit  # 0= 8bit  1=10bit (half rate)

	# NOTE 40MHz seems to yield the best result in terms of noise etc
	setup_clock_gen $CLOCK_GEN_OUT_AUDIO '40MHz'
}

function setup_video_card
{
	local sysfs_dir="$(card_sysfs cxadc${CXCARD_VIDEO_DEVICE})"
	if [[ ! -d "$sysfs_dir" ]] ; then die "Can't find video cxadc card at '$sysfs_dir'" ; fi

	echo $CXCARD_VIDEO_VMUX   > $sysfs_dir/vmux
	echo 0                    > $sysfs_dir/sixdb   # 0 off 1 +6db
	echo $CXCARD_VIDEO_LEVEL  > $sysfs_dir/level   # 0 min gain ... 31 max gain 
	echo 0                    > $sysfs_dir/tenxfsc # 0=1.0  1=1.24  2=1.4
	echo 0                    > $sysfs_dir/tenbit  # 0= 8bit  1=10bit (half rate)

	# NOTE 40MHz seems to yield the best result in terms of noise etc
	setup_clock_gen $CLOCK_GEN_OUT_VIDEO '40MHz'
}

function downsample_4_u8
{
	# https://sox.sourceforge.net/sox.html
	# https://stackoverflow.com/questions/1768077/how-can-i-make-sure-sox-doesnt-perform-automatic-dithering-without-knowing-the

	# About sox quality controls, from https://community.audirvana.com/t/explanation-for-sox-filter-controls/10848/9
	#       Quality   Band-  Rej dB   Typical Use
	#                 width
	# -q     quick     n/a   ~=30 @   playback on
	#                         Fs/4    ancient hardware
	# -l      low      80%    100     playback on old
	#                                 hardware
	# -m    medium     95%    100     audio playback
	# -h     high      95%    125     16-bit mastering
	#                                 (use with dither)
	# -v   very high   95%    175     24-bit mastering

	# Performance tests (SoX v14.4.2 / Linux 5.15.0-84-generic x86_64 / Ubuntu 22.04 / Intel(R) Core(TM) i5-4590 CPU)
	# - "-l" has about 50% usage of a single CPU core @ 40MSps -> 10MSps downsample
	#        10 MSps at 80% BW should give 4MHz analog signal bandwidth -> plenty for HiFi audio
	#        Manually confirmed to be correct by downsampling a sine sweep -> -6dB at 80% :)

	sox -D \
		-t raw -r 400000 -b 8 -c 1 -L -e unsigned-integer - \
		-t raw           -b 8 -c 1 -L -e unsigned-integer - rate -l 100000
}

function wait_for_ctrl_c
{
	local keep_running=true

	echo "Press Ctrl+C to stop recording"
	trap "keep_running=false" SIGINT

	while [[ $keep_running == true ]] ; do
		sleep 1
	done

	trap - SIGINT
	echo ""
}

function do_capture
{
	local output_dir="$1"

	local date_iso_now=$(date +%Y%m%d-%H%M%S)

	local alsa_sample_rate=46875 # 48000 is possible but has reduced quality
	local file_rf_video="$output_dir/$date_iso_now-rf-video-40msps.u8"
	local file_rf_audio="$output_dir/$date_iso_now-rf-audio-10msps.u8"
	local file_linear_audio="$output_dir/$date_iso_now-linear-audio-${alsa_sample_rate}sps-3ch-24bit-le.wav"

	pid_0=0
	pid_1=0
	pid_2=0

	local rf_buffer_size=256m # about 6 seconds of buffer on the rf streams at 40MByte/s

	cat /dev/cxadc${CXCARD_VIDEO_DEVICE} | pv --timer --rate --bytes --buffer-size $rf_buffer_size > "$file_rf_video" &
	pid_0=$!
	echo "Capturing to '$file_rf_video'"

	cat /dev/cxadc${CXCARD_AUDIO_DEVICE} | pv --timer --rate --bytes --buffer-size $rf_buffer_size | downsample_4_u8 > "$file_rf_audio" &
	pid_1=$!
	echo "Capturing to '$file_rf_audio'"
	
	local alsa_period=12000           # about 250ms / 4-times per sec.
	local alsa_buffer=$((48000 * 5))  # about 5 seconds of ALSA buffer
	arecord -D $CLOCK_GEN_ALSA_DEVICE -c 3 -r $alsa_sample_rate -f S24_3LE --period-size=$alsa_period --buffer-size=$alsa_buffer "$file_linear_audio" 2>&1 | grep -v "Aborted by signal Interrupt" &
	pid_2=$!
	echo "Capturing to '$file_linear_audio'"
	
	wait_for_ctrl_c

	for pid in $pid_0 $pid_1 $pid_2 ; do
		kill -SIGINT $pid 2>&1 | grep -v "No such process"
		echo "Signal pid $pid"
	done

	for pid in $pid_0 $pid_1 $pid_2 ; do
		wait $pid
		echo "End pid $pid"
	done

	echo "Done capturing :D"
}

function sanity_checks
{
	arecord --version | grep -q "Jaroslav" || die "arecord does not seem to be installed"
	arecord -L | grep -q "^$CLOCK_GEN_ALSA_DEVICE" || die "arecrod can't find the clock gen '$CLOCK_GEN_ALSA_DEVICE' check that device is plugged in, and user $(whoami) is in 'audio' group"
	amixer --version | grep -q "amixer version" || die "amixer does not seem to be installed"
	sox --version | grep -q "SoX" || die "SoX does not seem to be installed"
	pv --version | grep -q "Andrew" || die "pv does not seem to be installed"
}

function usage
{
	local url="https://gitlab.com/wolfre/cxadc-clock-generator-audio-adc"

	echo "A script to capture 3 streams of VHS in sync"
	echo "Copyright (c) Rene Wolf"
	echo ""
	echo "Expects to have a $url installed, and ready as '$CLOCK_GEN_ALSA_DEVICE'"
	echo ""
	echo "Usage:"
	echo "./$MY_NAME /media/lots-of-space/1984-oceania-holiday-tape"
	echo "   Will record RF streams from 2 CX cards and a 3ch wav from linear audio."
	echo "   All stored as individual files into the given directory."
	echo ""
	echo "For more details, see $url"
}

output_dir="$1"

if [[ "$output_dir" == "--help" ]] || [[ "$output_dir" == "-h" ]] ; then usage ; exit 0 ; fi
if [[ "$output_dir" == "" ]] ; then die "Need an output directory, see --help for help" ; fi
if [[ ! -d "$output_dir" ]] ; then die "Output directory '$output_dir' does not exist" ; fi


sanity_checks
setup_video_card
setup_audio_card

do_capture "$output_dir"
