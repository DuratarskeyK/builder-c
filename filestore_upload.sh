#!/bin/bash

urlencode() {
    # urlencode <string>
    old_lc_collate=$LC_COLLATE
    LC_COLLATE=C
    
    local length="${#1}"
    for (( i = 0; i < length; i++ )); do
        local c="${1:i:1}"
        case $c in
            [a-zA-Z0-9.~_-]) printf "$c" ;;
            *) printf '%%%02X' "'$c" ;;
        esac
    done
    
    LC_COLLATE=$old_lc_collate
}

if [ "$#" -lt 2 ]
then
	echo "Usage: $0 api_token folder"
	exit 1
fi

echo -n '[' > /tmp/results.json

start=0
for file in ${2}/*
do
	if [ "$file" = "${2}/*" ]
	then
		break
	fi

	if [ $start -eq 1 ] && [[ "$file" != *.rpm ]]
	then
		echo -n ',' >> /tmp/results.json
	fi
	encoded_name=$(urlencode `basename $file`)
	sha1=$(sha1sum $file | awk '{print $1}')
	while :
	do
		curl_ret=$(curl -s http://file-store.openmandriva.org/api/v1/file_stores.json?hash=$sha1)
		if [ "$curl_ret" != '[]' ]
		then
			filename=$(basename $file)
			if [[ "$filename" != *.rpm ]]
			then
				size=$(stat --printf="%s" $file)
				size_mb=$(awk 'BEGIN {printf "%.2f",'$size'/1048576}')
				echo -n "{\"sha1\":\"$sha1\",\"file_name\":\"$filename\",\"size\":\"$size_mb\"}" >> /tmp/results.json
				start=1
			fi
			break
		fi
		curl -s --user ${1}: -POST -F "file_store[file]=@${file};filename=${encoded_name}" http://file-store.openmandriva.org/api/v1/upload > /dev/null
	done
done
echo -n "]" >> /tmp/results.json