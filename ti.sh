#!/bin/bash

# Parse the arguments
GIT_SRC="britishamateurtelevisionclub";  # default
LMNDE="false"                            # don't load LimeSuiteNG and LimeNET Micro 2.0 DE specifics unless requested
WAIT="false"                             # Go straight into reboot unless requested
POSITIONAL_ARGS=()
while [[ $# -gt 0 ]]; do
  case $1 in
    -d|--development)
      echo -d
      if [[ "$GIT_SRC" == "britishamateurtelevisionclub" ]]; then
        GIT_SRC="davecrump"
      fi
      shift # past argument
      ;;
    -w|--wait)
      echo -w
      WAIT="true"
      shift # past argument
      ;;
    -x|--xtrx)
      echo -x
      LMNDE="true" 
      shift # past argument
      ;;
    -u|--user)
      echo -user
      shift # past argument
      echo user"$1"
      GIT_SRC=$1
      shift # past argument
      ;;
    -*|--*)
      echo "Unknown option $1"
      echo "Valid options:"
      echo "-d or --development          Build Portsdown 5 from development repository"
      echo "-w or --wait                 Wait at the end of Stage 1 of the build before reboot"
      echo "-x or --xtrx                 Include the files and modules for the LimeNET Micro 2.0 DE"
      echo "-u or --user githubname      Build Portsdown 5 from githubname/portsdown5 repository"
      exit 1
      ;;
    *)
      POSITIONAL_ARGS+=("$1") # save positional arg
      shift # past argument
      ;;
  esac
done
set -- "${POSITIONAL_ARGS[@]}" # restore positional parameters

echo GIT_SRC="$GIT_SRC";  # default
echo LMNDE="$LMNDE"                            # don't load LimeSuiteNG and LimeNET Micro 2.0 DE specifics unless requested
echo WAIT="$WAIT"                             # Go straight into reboot unless requested

echo done
exit