mkdir log -p
find src -regex ".*\.cpp\|.*\.h" | vera++ --show-rule --no-duplicate -c ./log/vera++-report.xml
