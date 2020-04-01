#!/bin/bash
set -x
source_path="${BASH_SOURCE[0]}"
source_dir=$(dirname "${source_path}")

rm "${source_dir}/san_miguel/san-miguel_"*
rm "${source_dir}/san_miguel/timings"*
rm "${source_dir}/san_miguel/samplerates"*

#MDLC with matrix sep
for i in {1..5}
do
	mitsuba "${source_dir}/san_miguel/san-miguel.xml" -Dspp=1 -Dcps=500 -Dst=0.01
	python3 "${source_dir}/../compare_err.py" "${source_dir}/san_miguel/san-miguel.exr" "${source_dir}/san_miguel/groundtruth.exr" "${source_dir}/san_miguel/san-miguel_matsep_500"
done
mv "${source_dir}/san_miguel/timings" "${source_dir}/san_miguel/timings_matsep_500"
mv "${source_dir}/san_miguel/samplerates" "${source_dir}/san_miguel/samplerates_matsep_500"

for i in {1..5}
do
	mitsuba "${source_dir}/san_miguel/san-miguel.xml" -Dspp=1 -Dcps=1000 -Dst=0.005
	python3 "${source_dir}/../compare_err.py" "${source_dir}/san_miguel/san-miguel.exr" "${source_dir}/san_miguel/groundtruth.exr" "${source_dir}/san_miguel/san-miguel_matsep_1000"
done
mv "${source_dir}/san_miguel/timings" "${source_dir}/san_miguel/timings_matsep_1000"
mv "${source_dir}/san_miguel/samplerates" "${source_dir}/san_miguel/samplerates_matsep_1000"

for i in {1..5}
do
	mitsuba "${source_dir}/san_miguel/san-miguel.xml" -Dspp=1 -Dcps=2000 -Dst=0.0025
	python3 "${source_dir}/../compare_err.py" "${source_dir}/san_miguel/san-miguel.exr" "${source_dir}/san_miguel/groundtruth.exr" "${source_dir}/san_miguel/san-miguel_matsep_2000"
done
mv "${source_dir}/san_miguel/timings" "${source_dir}/san_miguel/timings_matsep_2000"
mv "${source_dir}/san_miguel/samplerates" "${source_dir}/san_miguel/samplerates_matsep_2000"

for i in {1..5}
do
	mitsuba "${source_dir}/san_miguel/san-miguel.xml" -Dspp=1 -Dcps=4000 -Dst=0.00125
	python3 "${source_dir}/../compare_err.py" "${source_dir}/san_miguel/san-miguel.exr" "${source_dir}/san_miguel/groundtruth.exr" "${source_dir}/san_miguel/san-miguel_matsep_4000"
done
mv "${source_dir}/san_miguel/timings" "${source_dir}/san_miguel/timings_matsep_4000"
mv "${source_dir}/san_miguel/samplerates" "${source_dir}/san_miguel/samplerates_matsep_4000"

for i in {1..5}
do
	mitsuba "${source_dir}/san_miguel/san-miguel.xml" -Dspp=1 -Dcps=8000 -Dst=0.000625
	python3 "${source_dir}/../compare_err.py" "${source_dir}/san_miguel/san-miguel.exr" "${source_dir}/san_miguel/groundtruth.exr" "${source_dir}/san_miguel/san-miguel_matsep_8000"
done
mv "${source_dir}/san_miguel/timings" "${source_dir}/san_miguel/timings_matsep_8000"
mv "${source_dir}/san_miguel/samplerates" "${source_dir}/san_miguel/samplerates_matsep_8000"