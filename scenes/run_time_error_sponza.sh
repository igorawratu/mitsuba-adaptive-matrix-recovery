#!/bin/bash
set -x
source_path="${BASH_SOURCE[0]}"
source_dir=$(dirname "${source_path}")

rm "${source_dir}/sponza/sponza_"*
rm "${source_dir}/sponza/timings"*
rm "${source_dir}/sponza/samplerates"*

#MDLC with matrix sep
for i in {1..5}
do
	mitsuba "${source_dir}/sponza/sponza.xml" -Dspp=1 -Dcps=500 -st=0.002
	python3 "${source_dir}/../compare_err.py" "${source_dir}/sponza/sponza.exr" "${source_dir}/sponza/groundtruth.exr" "${source_dir}/sponza/sponza_matsep_500"
done
mv "${source_dir}/sponza/timings" "${source_dir}/sponza/timings_matsep_500"
mv "${source_dir}/sponza/samplerates" "${source_dir}/sponza/samplerates_matsep_500"

for i in {1..5}
do
	mitsuba "${source_dir}/sponza/sponza.xml" -Dspp=1 -Dcps=1000 -st=0.001
	python3 "${source_dir}/../compare_err.py" "${source_dir}/sponza/sponza.exr" "${source_dir}/sponza/groundtruth.exr" "${source_dir}/sponza/sponza_matsep_1000"
done
mv "${source_dir}/sponza/timings" "${source_dir}/sponza/timings_matsep_1000"
mv "${source_dir}/sponza/samplerates" "${source_dir}/sponza/samplerates_matsep_1000"

for i in {1..5}
do
	mitsuba "${source_dir}/sponza/sponza.xml" -Dspp=1 -Dcps=2000 -st=0.0005
	python3 "${source_dir}/../compare_err.py" "${source_dir}/sponza/sponza.exr" "${source_dir}/sponza/groundtruth.exr" "${source_dir}/sponza/sponza_matsep_2000"
done
mv "${source_dir}/sponza/timings" "${source_dir}/sponza/timings_matsep_2000"
mv "${source_dir}/sponza/samplerates" "${source_dir}/sponza/samplerates_matsep_2000"

for i in {1..5}
do
	mitsuba "${source_dir}/sponza/sponza.xml" -Dspp=1 -Dcps=4000 -st=0.00025
	python3 "${source_dir}/../compare_err.py" "${source_dir}/sponza/sponza.exr" "${source_dir}/sponza/groundtruth.exr" "${source_dir}/sponza/sponza_matsep_4000"
done
mv "${source_dir}/sponza/timings" "${source_dir}/sponza/timings_matsep_4000"
mv "${source_dir}/sponza/samplerates" "${source_dir}/sponza/samplerates_matsep_4000"

for i in {1..5}
do
	mitsuba "${source_dir}/sponza/sponza.xml" -Dspp=1 -Dcps=8000 -st=0.000125
	python3 "${source_dir}/../compare_err.py" "${source_dir}/sponza/sponza.exr" "${source_dir}/sponza/groundtruth.exr" "${source_dir}/sponza/sponza_matsep_8000"
done
mv "${source_dir}/sponza/timings" "${source_dir}/sponza/timings_matsep_8000"
mv "${source_dir}/sponza/samplerates" "${source_dir}/sponza/samplerates_matsep_8000"