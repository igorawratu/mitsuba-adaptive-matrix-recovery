#!/bin/bash
set -x
source_path="${BASH_SOURCE[0]}"
source_dir=$(dirname "${source_path}")

rm "${source_dir}/cbox/cbox_"*
rm "${source_dir}/cbox/timings"*
rm "${source_dir}/cbox/samplerates"*


#MDLC with matrix sep
for i in {1..5}
do
	mitsuba "${source_dir}/cbox/cbox.xml" -Dspp=1 -Dcps=500 -Dst=0
	convert -resize 50% "${source_dir}/cbox/cbox.exr" "${source_dir}/cbox/cbox.exr"
	python3 "${source_dir}/../compare_err.py" "${source_dir}/cbox/cbox.exr" "${source_dir}/cbox/groundtruth.exr" "${source_dir}/cbox/cbox_matsep_500"
done
mv "${source_dir}/cbox/timings" "${source_dir}/cbox/timings_matsep_500"
mv "${source_dir}/cbox/samplerates" "${source_dir}/cbox/samplerates_matsep_500"

for i in {1..5}
do
	mitsuba "${source_dir}/cbox/cbox.xml" -Dspp=1 -Dcps=1000 -Dst=0
	convert -resize 50% "${source_dir}/cbox/cbox.exr" "${source_dir}/cbox/cbox.exr"
	python3 "${source_dir}/../compare_err.py" "${source_dir}/cbox/cbox.exr" "${source_dir}/cbox/groundtruth.exr" "${source_dir}/cbox/cbox_matsep_1000"
done
mv "${source_dir}/cbox/timings" "${source_dir}/cbox/timings_matsep_1000"
mv "${source_dir}/cbox/samplerates" "${source_dir}/cbox/samplerates_matsep_1000"

for i in {1..5}
do
	mitsuba "${source_dir}/cbox/cbox.xml" -Dspp=1 -Dcps=2000 -Dst=0
	convert -resize 50% "${source_dir}/cbox/cbox.exr" "${source_dir}/cbox/cbox.exr"
	python3 "${source_dir}/../compare_err.py" "${source_dir}/cbox/cbox.exr" "${source_dir}/cbox/groundtruth.exr" "${source_dir}/cbox/cbox_matsep_2000"
done
mv "${source_dir}/cbox/timings" "${source_dir}/cbox/timings_matsep_2000"
mv "${source_dir}/cbox/samplerates" "${source_dir}/cbox/samplerates_matsep_2000"

for i in {1..5}
do
	mitsuba "${source_dir}/cbox/cbox.xml" -Dspp=1 -Dcps=4000 -Dst=0
	convert -resize 50% "${source_dir}/cbox/cbox.exr" "${source_dir}/cbox/cbox.exr"
	python3 "${source_dir}/../compare_err.py" "${source_dir}/cbox/cbox.exr" "${source_dir}/cbox/groundtruth.exr" "${source_dir}/cbox/cbox_matsep_4000"
done
mv "${source_dir}/cbox/timings" "${source_dir}/cbox/timings_matsep_4000"
mv "${source_dir}/cbox/samplerates" "${source_dir}/cbox/samplerates_matsep_4000"

for i in {1..5}
do
	mitsuba "${source_dir}/cbox/cbox.xml" -Dspp=1 -Dcps=8000 -Dst=0
	convert -resize 50% "${source_dir}/cbox/cbox.exr" "${source_dir}/cbox/cbox.exr"
	python3 "${source_dir}/../compare_err.py" "${source_dir}/cbox/cbox.exr" "${source_dir}/cbox/groundtruth.exr" "${source_dir}/cbox/cbox_matsep_8000"
done
mv "${source_dir}/cbox/timings" "${source_dir}/cbox/timings_matsep_8000"
mv "${source_dir}/cbox/samplerates" "${source_dir}/cbox/samplerates_matsep_8000"
