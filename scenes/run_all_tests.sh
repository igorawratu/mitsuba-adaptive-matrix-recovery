#!/bin/bash
set -x
source_path="${BASH_SOURCE[0]}"
source_dir=$(dirname "${source_path}")

chmod +x "${source_dir}/run_time_error_cbox.sh"
chmod +x "${source_dir}/run_time_error_breakfast-room.sh"
chmod +x "${source_dir}/run_time_error_classroom.sh"
chmod +x "${source_dir}/run_time_error_hairball.sh"
chmod +x "${source_dir}/run_time_error_living_room_3.sh"
chmod +x "${source_dir}/run_time_error_san_miguel.sh"
chmod +x "${source_dir}/run_time_error_sponza.sh"
chmod +x "${source_dir}/run_time_error_staircase.sh"

#${source_dir}/run_time_error_cbox.sh
#${source_dir}/run_time_error_breakfast-room.sh
#${source_dir}/run_time_error_classroom.sh
${source_dir}/run_time_error_hairball.sh
${source_dir}/run_time_error_living_room_3.sh
${source_dir}/run_time_error_san_miguel.sh
#${source_dir}/run_time_error_sponza.sh
#${source_dir}/run_time_error_staircase.sh