mkdir log -p
gcovr -v -x --object-directory=. -r . -o ./log/gcovr-report.xml
# gcovr --html --object-directory=. -r . -o ./log/gcovr-report.html
# gcovr --html-details --object-directory=. -r . -o ./log/gcovr-report-details.htm
