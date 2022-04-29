#consider the NDD case
path="/data/vendor/camera_dump/"
file_pattern="*p1.dump"
tool="camsys_dump_tool"
log_file_run="${path}/p1_reg_convert.log"
log_file_dump="${path}/p1_meta_ipi_cq_convert.log"

echo "" > ${log_file_run}
echo "" > ${log_file_dump}

for p in $(find $path -name $file_pattern); do
	dir=$(dirname "$p")
	base=$(basename "$p")
	output_name_run="${base%.*}-converted_p1.reg"
	output_name_dump="${base%.*}-meta_ipi_cq_p1.log"
	${tool} dump --print-meta --print-ipi --print-cq -o "${dir}/${output_name_dump}" -f ${p} 2>&1 >> ${log_file_dump}
	${tool} run -o "${dir}/${output_name_run}" -f ${p} 2>&1 >> ${log_file_run}
done

echo "${log_file_run} ${log_file_dump} created"