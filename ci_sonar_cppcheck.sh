mkdir log -p
cppcheck -v --enable=all --xml -Isrc src/ 2> ./log/cppcheck-report.xml
