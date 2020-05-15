#!/bin/sh
errorCatch() {
    printf "%s\n" '-> Something went wrong. Exiting'.
    exit 1
}

prepare_and_run() {
    . /etc/profile
    printf '%s\n' 'clone builder-c git project.'
#    cd
#    git clone https://github.com/DuratarskeyK/builder-c.git
    cd /root/builder-c/
    [ $? != '0' ] && errorCatch
    how-to-use-pvs-studio-free -c 2 -m .
    [ $? != '0' ] && errorCatch
    pvs-studio-analyzer trace -- bash build.sh
    [ $? != '0' ] && errorCatch
    pvs-studio-analyzer analyze -o /tmp/project.log -j 2 -C /usr/bin/clang
    [ $? != '0' ] && errorCatch
    plog-converter -a GA:1,2 -t fullhtml /tmp/project.log -o /output/
    [ $? != '0' ] && errorCatch
    cat /tmp/project.log
}

prepare_and_run
