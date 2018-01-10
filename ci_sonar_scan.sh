echo "Determining current branch and updating sonar-project.properties..."
CURRENT_BRANCH_GIT=`git describe --all | sed s/'heads\/'//g`
echo "Current branch: $CURRENT_BRANCH_GIT (CI: $CI_COMMIT_REF_NAME)"
# sed s/UNKNOWN_BRANCH/$CURRENT_BRANCH_GIT/g sonar-project.properties > sonar-project.properties.current
# mv sonar-project.properties sonar-project.properties.orig
# mv sonar-project.properties.current sonar-project.properties
# cat sonar-project.properties
echo "Starting scanner..."
# sonar-scanner -X -D sonar.branch=$CI_COMMIT_REF_NAME  # debug mode
sonar-scanner -D sonar.branch=$CI_COMMIT_REF_NAME       # regular mode
status=$?
if [ $status -eq 0 ]
then
  echo "Upload to sonar qube successfully completed."
else
  echo "sonar-scanner reported error code $status - exiting" >&2
  exit $status
fi

# echo "Restoring sonar-project.properties..."
# rm sonar-project.properties
# mv sonar-project.properties.orig sonar-project.properties
# echo "Done."
