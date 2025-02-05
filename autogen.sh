#!/bin/bash
SKIP=0
ENABLE_DEBUG=1    #Disables debug build flag.  
NO_DOWNLOAD=0
WGET_CMD="wget"   #Default wget command. Gets properly initialized later with parameters adequate for its version

#Save current working directory to run configure in
WORK_DIR=$(pwd)
#Get project root directory based on autogen.sh file location
SCRT_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)

SUBPROJECT_DIR=${SUBPROJECT_DIR:="$SCRT_DIR/subprojects"}

#Cache folder for downloaded sources
SRC_CACHE_DIR=$SUBPROJECT_DIR/.cache

# Define color code constants
YELLOW='\033[0;33m'
BHIGREEN='\x1b[38;5;118m'
RED='\033[0;31m'
NC='\033[0m' # No Color
CYAN='\033[0;36m'
GREEN='\033[0;32m'

#Failure marker
FAILED=0

script_start=$SECONDS

GSOAP_SRC_DIR="${GSOAP_SRC_DIR:=subprojects/gsoap-2.8}"

i=1;
for arg in "$@" 
do
    shift
    if [ "$arg" == "--enable-libav=yes" ] || [ "$arg" == "--enable-libav=true" ] || [ "$arg" == "--enable-libav" ]; then
        ENABLE_LIBAV=1
        set -- "$@" "$arg"
    elif [ "$arg" == "--enable-latest" ]; then
        ENABLE_LATEST=1
        set -- "$@" "$arg"
    elif [ "$arg" == "--no-debug" ]; then
        ENABLE_DEBUG=0
    elif [ "$arg" == "--enable-nvcodec=yes" ] || [ "$arg" == "--enable-nvcodec=true" ] || [ "$arg" == "--enable-nvcodec" ]; then
        ENABLE_NVCODEC=1
        set -- "$@" "$arg"
    elif [ "$arg" == "--no-download" ]; then
        NO_DOWNLOAD=1
    else
        set -- "$@" "$arg"
    fi
    i=$((i + 1));
done

#Set debug flag on current project
[ $ENABLE_DEBUG -eq 1 ] && set -- "$@" "--enable-debug=yes"

unbufferCall(){
  if [ ! -z "$UNBUFFER_COMMAND" ]; then
    eval "unbuffer ${@}"
  else
    script -efq -c "$(printf "%s" "${@}")"
  fi
}

##############################
# Function to decorate lines with project, task and color
# It handles targeted ANSI and VT codes to maintain appropriate decoration
#   Codes : \r, \n, ^[[1K, ^[[2K, ^[<n>G
##############################
CSI_G_REGEX='^\[([0-9][0-9]*)G' #CSI <n> G: Cursor horizontal absolute
VT_CLEARLINE_REGEX='^\[[{1}|{2}]K' #EL1 clearbol & EL2 clearline
printline(){
  local project task color padding prefix val escape_buff
  local "${@}"
  line="" #Reset variable before using it
  escape_marker=0
  escape_buff=""
  carriage_returned=1

  while IFS= read -rn1 data; do
    if [[ "$data" == *\* ]]; then #escape codes flag
      if [ ! -z "$escape_buff" ]; then #save previous parsed escape code for later flush
        line="$line""$escape_buff"
        escape_buff=""
      fi
      escape_marker=1 #Reset marker
    elif [ -z "$data" ]; then #Newline, Flush buffer with newline
      if [ $carriage_returned -eq 1 ]; then
        printf "${GREEN}${project}${CYAN} ${task}${color}${prefix}${padding}%s%s${NC}"$'\n'$'\r' "$line" "$escape_buff" >&2
      else
        printf "%s%s"$'\n'$'\r' "$line" "$escape_buff" >&2
      fi
      escape_marker=0
      escape_buff=""
      line=""
      carriage_returned=1
    elif [ "$data" == $'\r' ]; then #Carriage return. Flush buffer
      if [ $carriage_returned -eq 1 ]; then
        printf "${GREEN}${project}${CYAN} ${task}${color}${prefix}${padding}%s%s${NC}"$'\r' "$line" "$escape_buff" >&2
      else
        printf "%s%s"$'\r' "$line" "$escape_buff" >&2
      fi
      escape_marker=0
      escape_buff=""
      line=""
      carriage_returned=1
    elif [ $escape_marker -gt 0 ]; then #Seperated the rest of the condition to save on performance
      escape_buff+=$data
      ((escape_marker++))

      if [ $escape_marker -eq 4 ] && [[ "$escape_buff" =~ $VT_CLEARLINE_REGEX ]]; then
        printf "%s%s" "$line" "$escape_buff"
        escape_marker=0
        escape_buff=""
        line=""
        carriage_returned=1
      elif  [ $escape_marker -ge 4 ] && [[ $escape_buff =~ $CSI_G_REGEX ]]; then
        val=${BASH_REMATCH[1]}
        ((val+=${#project})) #Adjust adsolute positioning after project and task label
        ((val+=${#task}))
        ((val+=${#padding}))
        ((val++))
        escape_buff="["$val"G"
        printf "%s%s" "$line" "$escape_buff" #Print without decoaration since we moved after prefix
        line=""
        escape_marker=0
        escape_buff=""
        carriage_returned=0
      elif [ $escape_marker -ge 5 ]; then #No processing of any other escape code
        escape_marker=0
        line="$line""$escape_buff"
        escape_buff=""
      else
        continue;
      fi
    elif [ ! -z "$data" ]; then #Save regular character for later flush
      line="$line""$data"
    fi
  done;

  #Flush remaining buffers
  if [ ! -z "$line" ]; then
    printf "%s" "$line"
  fi
  if [ ! -z "$escape_buff" ]; then
    printf "%s" "$escape_buff"
  fi
}

printlines(){
  local project task msg color padding prefix
  local "${@}"

  #Init default color string
  [[ ! -z "${color}" ]] && incolor="${color}" || incolor="${NC}"

  #Init padding string
  if [[ -z "${padding}"  || ! "${padding}" == ?(-)+([[:digit:]]) ]]; then padding=1; fi
  inpadding="" && for i in {1..$padding}; do inpadding=$inpadding" "; done

  if [ ! -z "${msg}" ]; then
    echo "${msg}" | printline project="${project}" task="${task}" color="${incolor}" padding="${inpadding}" prefix="${prefix}"
  else
    printline project="${project}" task="${task}" color="${incolor}" padding="${inpadding}" prefix="${prefix}"
  fi
}

printError(){
  local msg project task
  local "${@}"
  printlines project="${project}" task="${task}" color=${RED} msg="*****************************"
  printlines project="${project}" task="${task}" color=${RED} prefix=" * " msg="${msg}"
  printlines project="${project}" task="${task}" color=${RED} prefix=" * " padding=4 msg="Kernel: $(uname -r)"
  printlines project="${project}" task="${task}" color=${RED} prefix=" * " padding=4 msg="Kernel: $(uname -i)"
  #Doesnt exist on REHL
  # printlines project="${project}" task="${task}" color=${RED} prefix=" * " padding=4 msg="$(lsb_release -a 2> /dev/null)"
  printlines project="${project}" task="${task}" color=${RED} msg="*****************************"
}

printNotice(){
  local msg project task color
  local "${@}"
  [[ ! -z "${color}" ]] && incolor="${color}" || incolor="${BHIGREEN}"
  printlines project="${project}" task="${task}" color=${incolor} msg="*****************************"
  printlines project="${project}" task="${task}" color=${incolor} prefix=" * " msg="${msg}"
  printlines project="${project}" task="${task}" color=${incolor} msg="*****************************"
}

############################################
#
# Function to print time for human
#
############################################
function displaytime {
  local time project task msg label
  local "${@}"
  local T=${time}
  local D=$((T/60/60/24))
  local H=$((T/60/60%24))
  local M=$((T/60%60))
  local S=$((T%60))
  [[ ! -z "${msg}" ]] && output="${msg}"$'\n'$'\n'$label || output="$label"
  (( $D > 0 )) && output=$output"$D days "
  (( $H > 0 )) && output=$output"$H hours "
  (( $M > 0 )) && output=$output"$M minutes "
  (( $D > 0 || $H > 0 || $M > 0 )) && output=$output"and "
  output=$output"$S seconds"
  printNotice project="${project}" task="${task}" msg="$output"
}

############################################
#
# Function to download and extract tar package file
# Can optionally use a caching folder defined with SRC_CACHE_DIR
#
############################################
downloadAndExtract (){
  local project path file forcedownload noexit # reset first
  local "${@}"

  if [ -z "${forcedownload}" ] && [ $NO_DOWNLOAD -eq 1 ]; then
    printError project="${project}" task="wget" msg="download disabled. Missing dependency $file"
    [[ ! -z "${noexit}" ]] && FAILED=1 && return || exit 1;
  fi

  [ ! -z "$SRC_CACHE_DIR" ] && pathprefix="$SRC_CACHE_DIR/" || pathprefix=""

  if [ ! -f "$dest_val" ]; then
    printlines project="${project}" task="wget" msg="downloading: ${path}"
    cd "$pathprefix" #Changing directory so that wget logs only the filename
    $WGET_CMD ${path} -O ${file} 2>&1 | printlines project="${project}" task="wget";
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
      printError project="${project}" task="wget" msg="failed to fetch ${path}"
      [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
    fi
    cd "$OLDPWD"
  else
    printlines project="${project}" task="wget" msg="source already downloaded"
  fi

  printlines project="${project}" task="wget" msg="extracting : ${file}"
  if [[ "${pathprefix}${file}" == *.tar.gz ]]; then
    tar xfz "${pathprefix}${file}" 2>&1 | printlines project="${project}" task="extract";
  elif [[ "${pathprefix}${file}" == *.tar.xz ]]; then
    tar xf "${pathprefix}${file}" 2>&1 | printlines project="${project}" task="extract";
  elif [[ "${pathprefix}${file}" == *.tar.bz2 ]]; then
    tar xjf "${pathprefix}${file}" 2>&1 | printlines project="${project}" task="extract";
  elif [[ "${pathprefix}${file}" == *.zip ]]; then
    checkUnzip
    unzip -oq "${pathprefix}${file}" 2>&1 | printlines project="${project}" task="extract";
  else
    printError project="${project}" task="wget" msg="downloaded file not found. ${file}"
    [[ ! -z "${noexit}" ]] && FAILED=1 && return || exit 1;
  fi
  if [ "${PIPESTATUS[0]}" -ne 0 ]; then
    printError project="${project}" task="extract" msg="failed to extract ${file}"
    [[ ! -z "${noexit}" ]] && FAILED=1 && return || exit 1;
  fi
}

############################################
#
# Function to clone a git repository or pull if it already exists locally
# Can optionally use a caching folder defined with SRC_CACHE_DIR
#
############################################
privatepullOrClone(){
  local project path dest tag recurse depth # reset first
  local "${@}"

  tgstr=""
  tgstr2=""
  if [ ! -z "${tag}" ] 
  then
    tgstr="origin tags/${tag}"
    tgstr2="-b ${tag}"
  fi

  recursestr=""
  if [ ! -z "${recurse}" ] 
  then
    recursestr="--recurse-submodules"
  fi
  depthstr=""
  if [ ! -z "${depth}" ] 
  then
    depthstr="--depth ${depth}"
  fi 
  
  should_clone=1
  if [ -d ${dest} ]; then
    unbufferCall "git -C ${dest} pull ${tgstr}" 2> /dev/null | printlines project="${project}" task="git"
    should_clone=${PIPESTATUS[0]}
    if [ $should_clone -ne 0 ]; then #Something failed. Remove to redownload
      rm -rf ${dest}
    fi
  fi

  if [ $should_clone -ne 0 ]; then
    unbufferCall "git clone -j$(nproc) $recursestr $depthstr $tgstr2 ${path} $dest" 2>&1 | printlines project="${project}" task="git"
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
      printError project="${project}" task="git" msg="failed to fetch ${path}"
      [[ ! -z "${noexit}" ]] && FAILED=1 && return || exit 1;
    fi
  fi
}

pullOrClone (){
  local project path tag depth recurse ignorecache forcedownload noexit # reset first
  local "${@}"

  if [ -z "${forcedownload}" ] && [ $NO_DOWNLOAD -eq 1 ]; then
    printError project="${project}" task="git" msg="Download disabled. Missing dependency $path"
    [[ ! -z "${noexit}" ]] && FAILED=1 && return || exit 1
  fi

  printlines project="${project}" task="git" msg="clone ${tag}@${path}"
  IFS='/' read -ra ADDR <<< "$path"
  namedotgit=${ADDR[-1]}
  IFS='.' read -ra ADDR <<< "$namedotgit"
  name=${ADDR[0]}

  dest_val=""
  if [ ! -z "$SRC_CACHE_DIR" ] && [ -z "${ignorecache}" ]; then
    dest_val="$SRC_CACHE_DIR/$name"
  else
    dest_val=$name
  fi
  if [ ! -z "${tag}" ]; then
    dest_val+="-${tag}"
  fi

  if [ -z "$SRC_CACHE_DIR" ] || [ -z "${ignorecache}" ]; then 
    #TODO Check if it's the right tag
    privatepullOrClone project="${project}" dest=$dest_val tag=$tag depth=$depth recurse=$recurse path=$path
  elif [ -d "$dest_val" ] && [ ! -z "${tag}" ]; then #Folder exist, switch to tag
    currenttag=$(git -C $dest_val tag --points-at ${tag})
    printlines project="${project}" task="git" msg="TODO Check current tag \"${tag}\" == \"$currenttag\""
    privatepullOrClone project="${project}" dest=$dest_val tag=$tag depth=$depth recurse=$recurse path=$path
  elif [ -d "$dest_val" ] && [ -z "${tag}" ]; then #Folder exist, switch to main
    currenttag=$(git -C $dest_val tag --points-at ${tag})
    printlines project="${project}" task="git" msg="TODO Handle no tag \"${tag}\" == \"$currenttag\""
    privatepullOrClone project="${project}" dest=$dest_val tag=$tag depth=$depth recurse=$recurse path=$path
  elif [ ! -d "$dest_val" ]; then #fresh start
    privatepullOrClone project="${project}" dest=$dest_val tag=$tag depth=$depth recurse=$recurse path=$path
  else
    if [ -d "$dest_val" ]; then
      printlines project="${project}" task="git" msg="yey destval"
    fi
    if [ -z "${tag}" ]; then
      printlines project="${project}" task="git" msg="yey tag"
    fi
    printlines project="${project}" task="git" msg="1 $dest_val : $(test -f \"$dest_val\")"
    printlines project="${project}" task="git" msg="2 ${tag} : $(test -z \"${tag}\")"
  fi

  if [ $FAILED -eq 0 ] && [ ! -z "$SRC_CACHE_DIR" ] && [ -z "${ignorecache}" ]; then
    printlines project="${project}" task="git" msg="copy repo from cache"$"$dest_val"
    rm -rf "$name"
    cp -r "$dest_val" "./$name"
  fi
}

############################################
#
# Function to build a project configured with autotools
#
############################################
buildMakeProject(){
  local project srcdir prefix autogen autoreconf configure make cmakedir cmakeargs cmakeclean installargs skipbootstrap bootstrap configcustom outoftree noexit
  local "${@}"

  build_start=$SECONDS

  printlines project="${project}" task="build" msg="src: '${srcdir}'"
  printlines project="${project}" task="build" msg="prefix: '${prefix}'"
  
  if [ "${outoftree}" == "true" ] 
  then
    mkdir -p "${srcdir}/build"
    cd "${srcdir}/build"
    rel_path=".."
  else
    cd "${srcdir}"
    rel_path="."
  fi

  if [ -f "$rel_path/bootstrap" ] && [ -z "${skipbootstrap}" ]; then
    printlines project="${project}" task="bootstrap" msg="src: ${srcdir}"
    unbufferCall "$rel_path/bootstrap ${bootstrap}" 2>&1 | printlines project="${project}" task="bootstrap"
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
      printError project="${project}" task="bootstrap" msg="$rel_path/bootstrap failed ${srcdir}"
      [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
    fi
  fi

  if [ -f "$rel_path/bootstrap.sh" ]; then
    printlines project="${project}" task="bootstrap.sh" msg="src: ${srcdir}"
    unbufferCall "$rel_path/bootstrap.sh ${bootstrap}" 2>&1 | printlines project="${project}" task="bootstrap.sh"
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
      printError project="${project}" task="bootstrap.sh" msg="$rel_path/bootstrap.sh failed ${srcdir}"
      [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
    fi
  fi
  if [ -f "$rel_path/autogen.sh" ] && [ "${autogen}" != "skip" ]; then
    printlines project="${project}" task="autogen.sh" msg="src: ${srcdir}"
    unbufferCall "$rel_path/autogen.sh ${autogen}" 2>&1 | printlines project="${project}" task="autogen.sh"
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
      printError project="${project}" task="autogen.sh" msg="Autogen failed ${srcdir}"
      [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
    fi
  fi

  if [ ! -z "${autoreconf}" ] 
  then
    printlines project="${project}" task="autoreconf" msg="src: ${srcdir}"
    unbufferCall "autoreconf ${autoreconf}" 2>&1 | printlines project="${project}" task="autoreconf"
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
      printError project="${project}" task="autoreconf" msg="Autoreconf failed ${srcdir}"
      [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
    fi
  fi
  
  if [ ! -z "${cmakedir}" ] 
  then
    printlines project="${project}" task="cmake" msg="src: ${srcdir}"
    printlines project="${project}" task="cmake" msg="arguments: ${cmakeargs}"
    if [ ! -z "${cmakeclean}" ]
    then 
      unbufferCall "cmake --build \"${cmakedir}\" --target clean" 2>&1 | printlines project="${project}" task="cmake"
      find . -iwholename '*cmake*' -not -name CMakeLists.txt -delete
    fi

    btype="Release"
    if [ $ENABLE_DEBUG -eq 1 ]; then
      btype="Debug"
    fi
    unbufferCall "cmake -G 'Unix Makefiles' " \
      "${cmakeargs} " \
      "-DCMAKE_BUILD_TYPE=$btype " \
      "-DCMAKE_INSTALL_PREFIX='${prefix}' " \
      "-DENABLE_TESTS=OFF " \
      "-DENABLE_SHARED=on " \
      "'${cmakedir}'" 2>&1 | printlines project="${project}" task="cmake"
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
      printError project="${project}" task="cmake" msg="failed ${srcdir}"
      [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
    fi
  fi

  if [ ! -z "${configcustom}" ]; then
    printlines project="${project}" task="bash" msg="src: ${srcdir}"
    printlines project="${project}" task="bash" msg="command: ${configcustom}"
    unbufferCall "bash -c \"${configcustom}\"" 2>&1 | printlines project="${project}" task="bash"
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
      printError project="${project}" task="bash" msg="command failed: ${configcustom}"
      [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
    fi
  fi

  if [ -f "$rel_path/configure" ] && [ -z "${cmakedir}" ]; then
    printlines project="${project}" task="configure" msg="src: ${srcdir}"
    printlines project="${project}" task="configure" msg="arguments: ${configure}"
    unbufferCall "$rel_path/configure " \
        "--prefix='${prefix}' " \
        "${configure}" 2>&1 | printlines project="${project}" task="configure"
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
      printError project="${project}" task="configure" msg="$rel_path/configure failed ${srcdir}"
      [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
    fi
  else
    printlines project="${project}" task="configure" msg="no configuration available"
  fi

  printlines project="${project}" task="compile" msg="src: ${srcdir}"
  printlines project="${project}" task="compile" msg="arguments: '${makeargs}'"
  unbufferCall "make -j$(nproc) ${make}" 2>&1 | printlines project="${project}" task="compile"
  if [ "${PIPESTATUS[0]}" -ne 0 ]; then
    printError project="${project}" task="compile" msg="make failed ${srcdir}"
    [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
  fi

  printlines project="${project}" task="install" msg="src: ${srcdir}"
  printlines project="${project}" task="install" msg="arguments: ${installargs}"
  unbufferCall "make -j$(nproc) ${make} install ${installargs}" 2>&1 | printlines project="${project}" task="install"
  if [ "${PIPESTATUS[0]}" -ne 0 ]; then
    printError project="${project}" task="install" msg="failed ${srcdir}"
    [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
  fi

  build_time=$(( SECONDS - build_start ))
  displaytime project="${project}" task="build" time=$build_time label="Build runtime: "

  cd "$OLDPWD"
}

############################################
#
# Function to build a project configured with meson
#
############################################
buildMesonProject() {
  local project srcdir mesonargs prefix setuppatch bindir destdir builddir defaultlib clean noexit
  local "${@}"

  build_start=$SECONDS

  printlines project="${project}" task="meson" msg="src: '${srcdir}'"
  printlines project="${project}" task="meson" msg="prefix: '${prefix}'"

  curr_dir=$(pwd)

  build_dir=""
  if [ ! -z "${builddir}" ] 
  then
    build_dir="${builddir}"
  else
    build_dir="build_dir"
  fi

  default_lib=""
  if [ ! -z "${defaultlib}" ] 
  then
    default_lib="${defaultlib}"
  else
    default_lib="static"
  fi

  bindir_val=""
  if [ ! -z "${bindir}" ]; then
      bindir_val="--bindir=${bindir}"
  fi
  
  if [ ! -z "${clean}" ]; then
    rm -rf "${srcdir}/$build_dir"
  fi

  if [ ! -d "${srcdir}/$build_dir" ]; then
      mkdir -p "${srcdir}/$build_dir"

      cd "${srcdir}"
      if [ -d "./subprojects" ]; then
        printlines project="${project}" task="meson" msg="download subprojects ${srcdir}"
        #     meson subprojects download
      fi

      if [ ! -z "${setuppatch}" ]; then
        printlines project="${project}" task="bash" msg="src: ${srcdir}"
        printlines project="${project}" task="bash" msg="command: ${setuppatch}"
        unbufferCall "bash -c \"${setuppatch}\"" 2>&1 | printlines project="${project}" task="bash"
        if [ "${PIPESTATUS[0]}" -ne 0 ]; then
          printError project="${project}" task="bash" msg="command failed: ${srcdir}"
          [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
        fi
      fi
      btype="release"
      if [ $ENABLE_DEBUG -eq 1 ]; then
        btype="debug"
      fi
      printlines project="${project}" task="meson" msg="setup ${srcdir}"
      unbufferCall "meson setup $build_dir " \
          "${mesonargs} " \
          "--default-library=$default_lib " \
          "--prefix='${prefix}' " \
          "$bindir_val " \
          "--libdir=lib " \
          "--includedir=include " \
          "--buildtype=$btype" 2>&1 | printlines project="${project}" task="meson"
      if [ "${PIPESTATUS[0]}" -ne 0 ]; then
        printError project="${project}" task="meson" msg="setup failed ${srcdir}"
        rm -rf "$build_dir"
        [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
      fi
  else
      cd "${srcdir}"
      if [ -d "./subprojects" ]; then
        printlines project="${project}" task="meson" msg="update ${srcdir}"
        #     meson subprojects update
      fi

      if [ ! -z "${setuppatch}" ]; then
        printlines project="${project}" task="bash" msg="command: ${setuppatch}"
        unbufferCall "bash -c \"${setuppatch}\"" 2>&1 | printlines project="${project}" task="bash"
        if [ "${PIPESTATUS[0]}" -ne 0 ]; then
          printError project="${project}" task="bash" msg="command failed ${srcdir}"
          [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
        fi
      fi
      btype="release"
      if [ $ENABLE_DEBUG -eq 1 ]; then
        btype="debug"
      fi
      printlines project="${project}" task="meson" msg="reconfigure ${srcdir}"
      unbufferCall "meson setup $build_dir " \
          "${mesonargs} " \
          "--default-library=$default_lib " \
          "--prefix='${prefix}' " \
          "$bindir_val " \
          "--libdir=lib " \
          "--includedir=include " \
          "--buildtype=$btype " \
          "--reconfigure" 2>&1 | printlines project="${project}" task="meson"
      if [ "${PIPESTATUS[0]}" -ne 0 ]; then
        printError project="${project}" task="meson" msg="setup failed ${srcdir}"
        rm -rf "$build_dir"
        [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
      fi
  fi

  printlines project="${project}" task="meson" msg="compile ${srcdir}"
  printf '\033[?7l' #Small hack to prevent meson from cropping progress line
  unbufferCall "meson compile -C $build_dir" 2>&1 | printlines project="${project}" task="compile"
  status="${PIPESTATUS[0]}"
  printf '\033[?7h' #Small hack to prevent meson from cropping progress line
  if [ $status -ne 0 ]; then
    printError project="${project}" task="meson" msg="compile failed ${srcdir}"
    [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
  fi

  printlines project="${project}" task="meson" msg="install ${srcdir}"
  DESTDIR=${destdir} unbufferCall "meson install -C $build_dir" 2>&1 | printlines project="${project}" task="install"
  if [ "${PIPESTATUS[0]}" -ne 0 ]; then
    printError project="${project}" task="meson" msg="install failed ${srcdir}"
    [[ ! -z "${noexit}" ]] && FAILED=1 && cd "$OLDPWD" && return || exit 1;
  fi

  build_time=$(( SECONDS - build_start ))
  displaytime project="${project}" task="build" time=$build_time label="Build runtime: "
  cd "$OLDPWD"

}

pkgCheck() {
  local name minver project
  local "${@}"

  min_ver=""
  if [ ! -z ${minver} ]; then
    min_ver=" >= ${minver}"
  fi
  output=$(pkg-config --print-errors --errors-to-stdout "${name} $min_ver");
  if [ -z "$output" ]; then
    printlines project="${project}" task="check" msg="found"
  else
    printlines project="${project}" task="check" msg="not found"
    echo 1
  fi
}

progVersionCheck(){
  local major minor micro linenumber lineindex program paramoverride startcut length project
  local "${@}"

  varparam="--version"
  if [ ! -z ${paramoverride} ]; then 
    varparam="${paramoverride}"
  fi;
  # echo "------------------------" 1> /dev/stdin
  # echo "test ${program} ${varparam}" 1> /dev/stdin
  # echo "------------------------" 1> /dev/stdin

  test=$(${program} $varparam 2> /dev/null)
  ret=$?
  if [[ $ret -ne 0 ]]; then
      printlines project="${project}" task="check" msg="not found"
      echo 1
      return
  fi

  FIRSTLINE=$(${program} ${varparam} | head -${linenumber})
  firstLineArray=(${FIRSTLINE//;/ })
  versionString=${firstLineArray[${lineindex}]};

  [[ ! -z "${startcut}" ]] && [[ -z "${length}" ]] && versionString=${versionString:2}
  [[ ! -z "${startcut}" ]] && [[ ! -z "${length}" ]] && versionString=${versionString:$startcut:$length}
  [[ -z "${startcut}" ]] && [[ ! -z "${length}" ]] && versionString=${versionString:0:$length}

  versionArray=(${versionString//./ })
  actualmajor=${versionArray[0]}
  actualminor=${versionArray[1]}
  actualmicro=${versionArray[2]}

  if [ -z "$major"  ]; then 
    printlines project="${project}" task="check" msg="found"
    return; 
  fi

  if [[ -z "$actualmajor"  || ! "$actualmajor" == ?(-)+([[:digit:]]) ]]; then echo "Unexpected ${program} version ouput[1] \"$FIRSTLINE\" \"$versionString\""; return; fi
  if [ "$actualmajor" -gt "$major" ]; then
    printlines project="${project}" task="check" msg="found"
    return;
  elif [ "$actualmajor" -lt "$major" ]; then
    printlines project="${project}" task="check" msg="version is too old. $actualmajor.$actualminor.$actualmicro < $major.$minor.$micro"
    echo 1
  else
    if [ -z "$minor"  ]; then return; fi
    if [[ -z "$actualminor"  || ! "$actualminor" == ?(-)+([[:digit:]]) ]]; then echo "Unexpected ${program} version ouput[2] \"$FIRSTLINE\" \"$versionString\""; return; fi
    if [ "$actualminor" -gt "$minor" ]; then
      printlines project="${project}" task="check" msg="found"
      return;
    elif [ "$actualminor" -lt "$minor" ]; then
      printlines project="${project}" task="check" msg="version is too old. $actualmajor.$actualminor.$actualmicro < $major.$minor.$micro"
      echo 1
    else
      if [ -z "$micro"  ]; then return; fi
      if [[ -z "$actualmicro"  || ! "$actualmicro" == ?(-)+([[:digit:]]) ]]; then echo "Unexpected ${program} version ouput[3] \"$FIRSTLINE\" \"$versionString\""; return; fi
      if [ ! -z "$micro" ] && [ "$actualmicro" -lt "$micro" ]; then
        printlines project="${project}" task="check" msg="version is too old. $actualmajor.$actualminor.$actualmicro < $major.$minor.$micro"
        echo 1
      else
        printlines project="${project}" task="check" msg="found"
        return
      fi
    fi
  fi
}

function checkLibrary {
  local name static project
  local "${@}"

  [[ ! -z "${static}" ]] && staticstr=" -static" || staticstr=""

  output="$(gcc -l${name}${staticstr} 2>&1)";
  if [[ "$output" == *"undefined reference to \`main'"* ]]; then
    printlines project="${project}" task="check" msg="found"
  else
    printlines project="${project}" task="check" msg="not found"
    echo 1
  fi
}

export PATH="$SUBPROJECT_DIR/unzip60/build/dist/bin":$PATH
HAS_UNZIP=0
checkUnzip(){
  if [ $HAS_UNZIP -eq 0 ] && [ ! -z "$(progVersionCheck project="unzip" program=unzip paramoverride="-v")" ]; then
    downloadAndExtract project="unzip" file="unzip60.tar.gz" path="https://sourceforge.net/projects/infozip/files/UnZip%206.x%20%28latest%29/UnZip%206.0/unzip60.tar.gz/download"
    buildMakeProject project="unzip" srcdir="unzip60" make="-f unix/Makefile generic" installargs=" prefix=$SUBPROJECT_DIR/unzip60/build/dist MANDIR=$SUBPROJECT_DIR/unzip60/build/dist/share/man/man1 -f unix/Makefile"
  fi
  HAS_UNZIP=1
}

export PATH="$SUBPROJECT_DIR/gettext-0.21.1/dist/bin":$PATH
HAS_GETTEXT=0
checkGetTextAndAutoPoint(){
  if [ $HAS_GETTEXT -eq 0 ] && \
     ([ ! -z "$(progVersionCheck project="gettext" program=gettextize linenumber=1 lineindex=3 major=0 minor=9 micro=18 )" ] || 
      [ ! -z "$(progVersionCheck project="autopoint" program=autopoint linenumber=1 lineindex=3 major=0 minor=9 micro=18 )" ]); then
    downloadAndExtract project="gettext" file="gettext-0.21.1.tar.gz" path="https://ftp.gnu.org/pub/gnu/gettext/gettext-0.21.1.tar.gz"
    buildMakeProject project="gettext" srcdir="gettext-0.21.1" prefix="$SUBPROJECT_DIR/gettext-0.21.1/dist" autogen="skip"
  fi
  HAS_GETTEXT=1
}


export PERL5LIB="$SUBPROJECT_DIR/perl-5.40.0/lib":$PERL5LIB
export PATH="$SUBPROJECT_DIR/perl-5.40.0/build/bin":$PATH
HAS_PERL=0
checkPerl(){
  if [ $HAS_PERL -eq 0 ] && \
      [ ! -z "$(progVersionCheck project="perl" program=perl linenumber=2 lineindex=8 major=5 minor=38 micro=0 startcut=2 length=6)" ]; then
      downloadAndExtract project="perl" file="perl-5.40.0.tar.gz" path="https://www.cpan.org/src/5.0/perl-5.40.0.tar.gz"
      buildMakeProject project="perl" srcdir="perl-5.40.0" skipbootstrap="true" configcustom="./Configure -des -Dprefix=$SUBPROJECT_DIR/perl-5.40.0/build"
  fi
  HAS_PERL=1
}


export PATH="$SUBPROJECT_DIR/glibc-2.40/build/dist/bin":$PATH
export LIBRARY_PATH="$SUBPROJECT_DIR/glibc-2.40/build/dist/lib":$LIBRARY_PATH
HAS_GLIBC=0
checkGlibc(){
  if [ $HAS_GLIBC -eq 0 ] && \
      [ ! -z "$(checkLibrary project="glibc" name=c static=true)" ]; then
      downloadAndExtract project="glibc" file="glibc-2.40.tar.xz" path="https://ftp.gnu.org/gnu/glibc/glibc-2.40.tar.xz"
      buildMakeProject project="glibc" srcdir="glibc-2.40" prefix="$SUBPROJECT_DIR/glibc-2.40/build/dist" skipbootstrap="true" outoftree="true"
  fi
  HAS_GLIBC=1
}

checkGitRepoState(){
  local repo package
  local "${@}"
  
  ret=0
  git -C ${repo} remote update &> /dev/null
  LOCAL=$(git -C ${repo} rev-parse @ 2> /dev/null)
  REMOTE=$(git -C ${repo} rev-parse @{u} 2> /dev/null)
  BASE=$(git -C ${repo} merge-base @ @{u} 2> /dev/null)

  if [ ! -z "$LOCAL" ] && [ $LOCAL = $REMOTE ]; then
    printlines project="${package}" task="git" msg="${repo} is already up-to-date. Do nothing..."
  elif [ ! -z "$LOCAL" ] && [ $LOCAL = $BASE ]; then
    printlines project="${package}" task="git" msg="${repo} has new changes. Force rebuild..."
    ret=1
  elif [ ! -z "$LOCAL" ] && [ $REMOTE = $BASE ]; then
    printlines project="${package}" task="git" msg="${repo} has local changes. Doing nothing..."
  elif [ ! -z "$LOCAL" ]; then
    printlines project="${package}" task="git" msg="Error ${repo} is diverged."
    ret=-1
  else
    printlines project="${package}" task="git" msg="not found"
    ret=-1
  fi

  if [ $ret == 0 ] && [ ! -z "${package}" ] && [ ! -z "$(pkgCheck name=${package} project=${project})" ]; then
    ret=1
  fi

  echo $ret
}

# Hard dependency check
MISSING_DEP=0
if [ ! -z "$(progVersionCheck project="make" program='make')" ]; then
  MISSING_DEP=1
fi

if [ ! -z "$(progVersionCheck project="pkg-config" program=pkg-config)" ]; then
  MISSING_DEP=1
fi

if [ ! -z "$(progVersionCheck project="g++" program=g++)" ]; then
  MISSING_DEP=1
fi

if [ ! -z "$(progVersionCheck project="git" program=git)" ]; then
  MISSING_DEP=1
fi

if [ ! -z "$(progVersionCheck project="tar" program=tar)" ]; then
  MISSING_DEP=1
fi

if [ ! -z "$(progVersionCheck project="g++" program=g++)" ]; then
  MISSING_DEP=1
fi

if [ ! -z "$(progVersionCheck project="gcc" program=gcc)" ]; then
  MISSING_DEP=1
fi

if [ ! -z "$(progVersionCheck project="wget" program=wget)" ]; then
  MISSING_DEP=1
elif [ -z "$(progVersionCheck project="wget" program=wget linenumber=1 lineindex=2 major=2)" ]; then
  WGET_CMD="wget --force-progress" #Since version 2+ show-progress ins't supported
elif [ -z "$(progVersionCheck project="wget" program=wget linenumber=1 lineindex=2 major=1 minor=16)" ]; then
  WGET_CMD="wget --show-progress --progress=bar:force" #Since version 1.16 set show-progress
fi

if [ $MISSING_DEP -eq 1 ]; then
  exit 1
fi

unbuffer echo "test" 2> /dev/null 1> /dev/null
[ $? == 0 ] && UNBUFFER_COMMAND="unbuffer" && printlines project="unbuffer" task="check" msg="found" || printlines project="unbuffer" task="check" msg="not found"

mkdir -p "$SUBPROJECT_DIR"
mkdir -p "$SRC_CACHE_DIR"

cd "$SUBPROJECT_DIR"

export ACLOCAL_PATH="$ACLOCAL_PATH:/usr/share/aclocal" #When using locally built autoconf, we need to add system path
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:"$(pkg-config --variable pc_path pkg-config)" #When using locally built pkgconf, we need to add system path

################################################################
# 
#    Build cutils
#       
################################################################
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/CUtils/build/dist/lib/pkgconfig":$PKG_CONFIG_PATH
rebuild=$(checkGitRepoState project="cutils" repo="CUtils" package="cutils")

if [ $rebuild != 0 ]; then
  #OnvifSoapLib depends on it and will also require a rebuild
  rm -rf "$SUBPROJECT_DIR/CUtils/build"
  rm -rf "$SUBPROJECT_DIR/OnvifSoapLib/build"
  
  pullOrClone project="cutils" path=https://github.com/Quedale/CUtils.git ignorecache="true"
  #Clean up previous build in case of failure. This will prevent from falling back on the old version
  rm -rf "$SUBPROJECT_DIR/CUtils/build/dist/*"
  buildMakeProject project="cutils" srcdir="CUtils" prefix="$SUBPROJECT_DIR/CUtils/build/dist" cmakedir=".." outoftree=true cmakeclean=true
fi

################################################################
# 
#    Build gSoap
#       
################################################################
export PATH="$SUBPROJECT_DIR/gsoap-2.8/build/dist/bin":$PATH
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/gsoap-2.8/build/dist/lib/pkgconfig":$PKG_CONFIG_PATH
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/zlib-1.2.13/build/dist/lib/pkgconfig":$PKG_CONFIG_PATH
export PKG_CONFIG_PATH="$SUBPROJECT_DIR/libntlm-1.8/build/dist/lib/pkgconfig":$PKG_CONFIG_PATH
gsoap_version=2.8.134
if [ ! -z "$(pkgCheck project="gsoap" name=gsoap minver=$gsoap_version)" ]; then
    if [ ! -z "$(pkgCheck project="zlib" name=zlib minver=1.2.11)" ]; then
        downloadAndExtract project="zlib" file="zlib-1.2.13.tar.gz" path="https://www.zlib.net/zlib-1.2.13.tar.gz"
        buildMakeProject project="zlib" srcdir="zlib-1.2.13" prefix="$SUBPROJECT_DIR/zlib-1.2.13/build/dist"
    fi

    if [ ! -z "$(pkgCheck project="libntlm" name=libntlm minver=v1.5)" ]; then
        downloadAndExtract project="libntlm" file="libntlm-1.8.tar.gz" path="https://download-mirror.savannah.gnu.org/releases/libntlm/libntlm-1.8.tar.gz"
        buildMakeProject project="libntlm" srcdir="libntlm-1.8" skipbootstrap="true" prefix="$SUBPROJECT_DIR/libntlm-1.8/build/dist" configure="--enable-shared=no" #TODO support shared linking
    fi

    SSL_PREFIX=$(pkg-config --variable=prefix openssl)
    if [ "$SSL_PREFIX" != "/usr" ]; then
        SSL_INCLUDE=$(\pkg-config --variable=includedir openssl)
        SSL_LIBS=$(pkg-config --variable=libdir openssl)
    fi

    ZLIB_PREFIX=$(pkg-config --variable=prefix zlib)
    if [ "$ZLIB_PREFIX" != "/usr" ]; then
        ZLIB_INCLUDE=$(pkg-config --variable=includedir zlib)
        ZLIB_LIBS=$(pkg-config --variable=libdir zlib)
    fi

    inc_path=""
    if [ ! -z "$SSL_INCLUDE" ] && [ "$SSL_INCLUDE" != "/usr/include" ]; then
        if [ -z "$inc_path" ]; then
            inc_path="$SSL_INCLUDE"
        else
            inc_path="$inc_path:$SSL_INCLUDE"
        fi
    fi
    if [ ! -z "$ZLIB_INCLUDE" ] && [ "$SSL_INCLUDE" != "/usr/include" ]; then
        if [ -z "$inc_path" ]; then
            inc_path="$ZLIB_INCLUDE"
        else
            inc_path="$inc_path:$ZLIB_INCLUDE"
        fi
    fi
    if [ ! -z "$CPLUS_INCLUDE_PATH" ] && [ "$SSL_INCLUDE" != "/usr/include" ]; then
        if [ -z "$inc_path" ]; then
            inc_path="$CPLUS_INCLUDE_PATH"
        else
            inc_path="$inc_path:$CPLUS_INCLUDE_PATH"
        fi
    fi

    rm -rf "$SUBPROJECT_DIR/gsoap-2.8/" #Make sure we dont extract over an old version
    downloadAndExtract project="gsoap" file="gsoap_$gsoap_version.zip" path="https://sourceforge.net/projects/gsoap2/files/gsoap_$gsoap_version.zip/download"
    C_INCLUDE_PATH="$inc_path" \
    CPLUS_INCLUDE_PATH="$inc_path" \
    LIBRARY_PATH="$SSL_LIBS:$ZLIB_LIBS:$LIBRARY_PATH" \
    LD_LIBRARY_PATH="$SSL_LIBS:$ZLIB_LIBS:$LD_LIBRARY_PATH" \
    LIBS='-ldl -lpthread' \
    buildMakeProject project="gsoap" srcdir="gsoap-2.8" prefix="$SUBPROJECT_DIR/gsoap-2.8/build/dist" autogen="skip" configure="--with-openssl=/usr/lib/ssl"
fi

# This option still required internet to reach oasis schemes.
# no internet requirement to support flatpak builder
# downloadAndExtract file="24.06.tar.gz" path="https://github.com/onvif/specs/archive/refs/tags/24.06.tar.gz"
# if [ $FAILED -eq 1 ]; then exit 1; fi

#Get out of subproject folder
cd ..


echo "Generating WSDL gsoap files..."
if [ ! -f $SUBPROJECT_DIR/../src/generated/onvif.h ]; then
  mkdir $SUBPROJECT_DIR/../src/generated
  wsdl2h -Ow4 -t$SUBPROJECT_DIR/../wsdl/typemap.dat -o $SUBPROJECT_DIR/../src/generated/onvif.h -cg \
      ./wsdl/onvif/wsdl/devicemgmt.wsdl \
      ./wsdl/onvif/wsdl/media.wsdl \
      ./wsdl/onvif/wsdl/remotediscovery.wsdl 
      # ./wsdl/onvif/wsdl/event.wsdl \
      # ./wsdl/onvif/wsdl/deviceio.wsdl \
      # ./wsdl/onvif/wsdl/imaging.wsdl \
      # ./wsdl/onvif/wsdl/media2.wsdl \
      # ./wsdl/onvif/wsdl/ptz.wsdl \
      # ./wsdl/onvif/wsdl/advancedsecurity.wsdl
      # Use bellow if using github ONVIF specs
      # ./subprojects/specs-24.06/wsdl/ver10/device/wsdl/devicemgmt.wsdl \
      # ./subprojects/specs-24.06/wsdl/ver10/events/wsdl/event.wsdl \
      # ./subprojects/specs-24.06/wsdl/ver10/deviceio.wsdl \
      # ./subprojects/specs-24.06/wsdl/ver20/imaging/wsdl/imaging.wsdl \
      # ./subprojects/specs-24.06/wsdl/ver10/media/wsdl/media.wsdl \
      # ./subprojects/specs-24.06/wsdl/ver20/media/wsdl/media.wsdl \
      # ./subprojects/specs-24.06/wsdl/ver20/ptz/wsdl/ptz.wsdl \
      # ./subprojects/specs-24.06/wsdl/ver10/advancedsecurity/wsdl/advancedsecurity.wsdl \
      
      # Use below for ONVIF schema from onvif.org
      # http://schemas.xmlsoap.org/ws/2005/04/discovery/ws-discovery.wsdl #ONVIF remotediscovery.wsdl is outdated https://github.com/onvif/specs/issues/319
      # http://www.onvif.org/onvif/ver10/device/wsdl/devicemgmt.wsdl \
      # http://www.onvif.org/onvif/ver10/events/wsdl/event.wsdl \
      # http://www.onvif.org/onvif/ver10/deviceio.wsdl \
      # http://www.onvif.org/onvif/ver20/imaging/wsdl/imaging.wsdl \
      # http://www.onvif.org/onvif/ver10/media/wsdl/media.wsdl \
      # http://www.onvif.org/onvif/ver20/media/wsdl/media.wsdl \
      # http://www.onvif.org/onvif/ver20/ptz/wsdl/ptz.wsdl \
      # http://www.onvif.org/onvif/ver10/network/wsdl/remotediscovery.wsdl \
      # http://www.onvif.org/ver10/advancedsecurity/wsdl/advancedsecurity.wsdl
  ret=$?
  if [ $ret != 0 ]; then
    rm -rf $SUBPROJECT_DIR/../src/generated
    printf "${RED}*****************************\n${NC}"
    printf "${RED}*** Failed to generate gsoap C header file ${srcdir} ***\n${NC}"
    printf "${RED}*****************************\n${NC}"
    exit 1;
  fi
  # http://www.onvif.org/onvif/ver10/display.wsdl \
  # http://www.onvif.org/onvif/ver10/receiver.wsdl \
  # http://www.onvif.org/onvif/ver10/recording.wsdl \
  # http://www.onvif.org/onvif/ver10/search.wsdl \
  # http://www.onvif.org/onvif/ver10/replay.wsdl \
  # http://www.onvif.org/onvif/ver20/analytics/wsdl/analytics.wsdl \
  # http://www.onvif.org/onvif/ver10/analyticsdevice.wsdl \ 
  soapcpp2 -n -ponvifsoap -f100 -SL -x -c -2 -I$GSOAP_SRC_DIR/gsoap/import:$GSOAP_SRC_DIR/gsoap $SUBPROJECT_DIR/../src/generated/onvif.h -d$SUBPROJECT_DIR/../src/generated
  ret=$?
  if [ $ret != 0 ]; then
    rm -rf $SUBPROJECT_DIR/../src/generated
    printf "${RED}*****************************\n${NC}"
    printf "${RED}*** Failed to generate gsoap C code ${srcdir} ***\n${NC}"
    printf "${RED}*****************************\n${NC}"
    exit 1;
  fi
fi