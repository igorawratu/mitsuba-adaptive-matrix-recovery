#!/bin/bash
set -x
source_path="${BASH_SOURCE[0]}"
source_dir=$(dirname "${source_path}")

rm "${source_dir}/breakfast-room/breakfast-room/scene_"*
rm "${source_dir}/breakfast-room/breakfast-room/timings"*
rm "${source_dir}/breakfast-room/breakfast-room/samplerates"*

#MDLC with matrix sep
for i in {1..5}
do
	mitsuba "${source_dir}/breakfast-room/breakfast-room/scene.xml" -Dspp=1 -Dcps=500 -Dst=0.002
	convert -resize 50% "${source_dir}/breakfast-room/breakfast-room/scene.exr" "${source_dir}/breakfast-room/breakfast-room/scene.exr"
	python3 "${source_dir}/../compare_err.py" "${source_dir}/breakfast-room/breakfast-room/scene.exr" "${source_dir}/breakfast-room/breakfast-room/groundtruth.exr" "${source_dir}/breakfast-room/breakfast-room/scene_matsep_500"
done
mv "${source_dir}/breakfast-room/breakfast-room/timings" "${source_dir}/breakfast-room/breakfast-room/timings_matsep_500"
mv "${source_dir}/breakfast-room/breakfast-room/samplerates" "${source_dir}/breakfast-room/breakfast-room/samplerates_matsep_500"

for i in {1..5}
do
	mitsuba "${source_dir}/breakfast-room/breakfast-room/scene.xml" -Dspp=1 -Dcps=1000 -Dst=0.001
	convert -resize 50% "${source_dir}/breakfast-room/breakfast-room/scene.exr" "${source_dir}/breakfast-room/breakfast-room/scene.exr"
	python3 "${source_dir}/../compare_err.py" "${source_dir}/breakfast-room/breakfast-room/scene.exr" "${source_dir}/breakfast-room/breakfast-room/groundtruth.exr" "${source_dir}/breakfast-room/breakfast-room/scene_matsep_1000"
done
mv "${source_dir}/breakfast-room/breakfast-room/timings" "${source_dir}/breakfast-room/breakfast-room/timings_matsep_1000"
mv "${source_dir}/breakfast-room/breakfast-room/samplerates" "${source_dir}/breakfast-room/breakfast-room/samplerates_matsep_1000"

for i in {1..5}
do
	mitsuba "${source_dir}/breakfast-room/breakfast-room/scene.xml" -Dspp=1 -Dcps=2000 -Dst=0.0005
	convert -resize 50% "${source_dir}/breakfast-room/breakfast-room/scene.exr" "${source_dir}/breakfast-room/breakfast-room/scene.exr"
	python3 "${source_dir}/../compare_err.py" "${source_dir}/breakfast-room/breakfast-room/scene.exr" "${source_dir}/breakfast-room/breakfast-room/groundtruth.exr" "${source_dir}/breakfast-room/breakfast-room/scene_matsep_2000"
done
mv "${source_dir}/breakfast-room/breakfast-room/timings" "${source_dir}/breakfast-room/breakfast-room/timings_matsep_2000"
mv "${source_dir}/breakfast-room/breakfast-room/samplerates" "${source_dir}/breakfast-room/breakfast-room/samplerates_matsep_2000"

for i in {1..5}
do
	mitsuba "${source_dir}/breakfast-room/breakfast-room/scene.xml" -Dspp=1 -Dcps=4000 -Dst=0.00025
	convert -resize 50% "${source_dir}/breakfast-room/breakfast-room/scene.exr" "${source_dir}/breakfast-room/breakfast-room/scene.exr"
	python3 "${source_dir}/../compare_err.py" "${source_dir}/breakfast-room/breakfast-room/scene.exr" "${source_dir}/breakfast-room/breakfast-room/groundtruth.exr" "${source_dir}/breakfast-room/breakfast-room/scene_matsep_4000"
done
mv "${source_dir}/breakfast-room/breakfast-room/timings" "${source_dir}/breakfast-room/breakfast-room/timings_matsep_4000"
mv "${source_dir}/breakfast-room/breakfast-room/samplerates" "${source_dir}/breakfast-room/breakfast-room/samplerates_matsep_4000"

for i in {1..5}
do
	mitsuba "${source_dir}/breakfast-room/breakfast-room/scene.xml" -Dspp=1 -Dcps=8000 -Dst=0.000125
	convert -resize 50% "${source_dir}/breakfast-room/breakfast-room/scene.exr" "${source_dir}/breakfast-room/breakfast-room/scene.exr"
	python3 "${source_dir}/../compare_err.py" "${source_dir}/breakfast-room/breakfast-room/scene.exr" "${source_dir}/breakfast-room/breakfast-room/groundtruth.exr" "${source_dir}/breakfast-room/breakfast-room/scene_matsep_8000"
done
mv "${source_dir}/breakfast-room/breakfast-room/timings" "${source_dir}/breakfast-room/breakfast-room/timings_matsep_8000"
mv "${source_dir}/breakfast-room/breakfast-room/samplerates" "${source_dir}/breakfast-room/breakfast-room/samplerates_matsep_8000"