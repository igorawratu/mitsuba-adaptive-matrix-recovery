#!/bin/bash
set -x
source_path="${BASH_SOURCE[0]}"
source_dir=$(dirname "${source_path}")

rm "${source_dir}/living_room_3/living-room-3/scene_"*
rm "${source_dir}/living_room_3/living-room-3/timings"*
rm "${source_dir}/living_room_3/living-room-3/samplerates"*

#MDLC with matrix sep
for i in {1..5}
do
	mitsuba "${source_dir}/living_room_3/living-room-3/scene.xml" -Dspp=1 -Dcps=500 -Dst=0.002
	python3 "${source_dir}/../compare_err.py" "${source_dir}/living_room_3/living-room-3/scene.exr" "${source_dir}/living_room_3/living-room-3/groundtruth.exr" "${source_dir}/living_room_3/living-room-3/scene_matsep_500"
done
mv "${source_dir}/living_room_3/living-room-3/timings" "${source_dir}/living_room_3/living-room-3/timings_matsep_500"
mv "${source_dir}/living_room_3/living-room-3/samplerates" "${source_dir}/living_room_3/living-room-3/samplerates_mdlc_500"

for i in {1..5}
do
	mitsuba "${source_dir}/living_room_3/living-room-3/scene.xml" -Dspp=1 -Dcps=1000 -Dst=0.001
	python3 "${source_dir}/../compare_err.py" "${source_dir}/living_room_3/living-room-3/scene.exr" "${source_dir}/living_room_3/living-room-3/groundtruth.exr" "${source_dir}/living_room_3/living-room-3/scene_matsep_1000"
done
mv "${source_dir}/living_room_3/living-room-3/timings" "${source_dir}/living_room_3/living-room-3/timings_matsep_1000"
mv "${source_dir}/living_room_3/living-room-3/samplerates" "${source_dir}/living_room_3/living-room-3/samplerates_matsep_1000"

for i in {1..5}
do
	mitsuba "${source_dir}/living_room_3/living-room-3/scene.xml" -Dspp=1 -Dcps=2000 -Dst=0.0005
	python3 "${source_dir}/../compare_err.py" "${source_dir}/living_room_3/living-room-3/scene.exr" "${source_dir}/living_room_3/living-room-3/groundtruth.exr" "${source_dir}/living_room_3/living-room-3/scene_matsep_2000"
done
mv "${source_dir}/living_room_3/living-room-3/timings" "${source_dir}/living_room_3/living-room-3/timings_matsep_2000"
mv "${source_dir}/living_room_3/living-room-3/samplerates" "${source_dir}/living_room_3/living-room-3/samplerates_matsep_2000"

for i in {1..5}
do
	mitsuba "${source_dir}/living_room_3/living-room-3/scene.xml" -Dspp=1 -Dcps=4000 -Dst=0.00025
	python3 "${source_dir}/../compare_err.py" "${source_dir}/living_room_3/living-room-3/scene.exr" "${source_dir}/living_room_3/living-room-3/groundtruth.exr" "${source_dir}/living_room_3/living-room-3/scene_matsep_4000"
done
mv "${source_dir}/living_room_3/living-room-3/timings" "${source_dir}/living_room_3/living-room-3/timings_matsep_4000"
mv "${source_dir}/living_room_3/living-room-3/samplerates" "${source_dir}/living_room_3/living-room-3/samplerates_matsep_4000"

for i in {1..5}
do
	mitsuba "${source_dir}/living_room_3/living-room-3/scene.xml" -Dspp=1 -Dcps=8000 -Dst=0.000125
	python3 "${source_dir}/../compare_err.py" "${source_dir}/living_room_3/living-room-3/scene.exr" "${source_dir}/living_room_3/living-room-3/groundtruth.exr" "${source_dir}/living_room_3/living-room-3/scene_matsep_8000"
done
mv "${source_dir}/living_room_3/living-room-3/timings" "${source_dir}/living_room_3/living-room-3/timings_matsep_8000"
mv "${source_dir}/living_room_3/living-room-3/samplerates" "${source_dir}/living_room_3/living-room-3/samplerates_matsep_8000"