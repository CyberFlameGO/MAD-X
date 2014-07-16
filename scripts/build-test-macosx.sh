#! /bin/bash
# run:
# bash scripts/build-test-macosx.sh [noecho] [cleanall] [notest]

# env settings
export LC_CTYPE="C"
export PATH="/Users/mad/Projects/madX:/opt/local/bin:$PATH"

# error handler
check_error ()
{
	if [ "$?" != "0" ] ; then
		echo "ERROR: $1"
		exit 1
	fi
}

# I/O redirection
rm -f build-test-macosx.out
if [ "$1" = "noecho" ] ; then
	shift
	exec > build-test-macosx.out 2>&1
	check_error "redirection with noecho failed"
else
	exec > >(tee build-test-macosx.out) 2> >(tee build-test-macosx.out >&2)
	check_error "redirection with tee failed"
fi

echo -e "\n===== Start of build and tests ====="
echo "Date  : `date`"
echo "System: `uname -m -n -r -s`"
echo "Script: $0 $@"

echo -e "\n===== SVN update ====="
svn update
if [ "$?" != "0" ] ; then
	echo -e "\n===== SVN cleanup & update ====="
	svn cleanup
	svn update
	check_error "svn update failed"
fi

echo -e "\n===== Release number ====="
cat VERSION

echo -e "\n===== Clean build ====="
if [ "$1" = "cleanall" ] ; then
	shift
	make cleanall
	check_error "make cleanall failed"
else
	echo "Skipped (no explicit request)."
fi 

echo -e "\n===== Gnu build ====="
gcc      --version
g++      --version
gfortran --version
make all-macosx-gnu
check_error "make all-macosx-gnu failed"

echo -e "\n===== Intel build ====="
icc      --version
ifort    --version
make all-macosx-intel all-macosx
check_error "make all-macosx-intel failed"

echo -e "\n===== Binaries dependencies ====="
make infobindep
check_error "make infobindep failed"

echo -e "\n===== Tests pointless files ====="
make cleantest && make infotestdep
check_error "make infotestdep failed"

echo -e "\n===== Running tests (long) ====="
if [ "$1" = "notest" ] ; then
	shift
	echo "Skipped (explicit request)."
else
	echo ""

	echo -e "\n===== Testing madx-macosx64-intel ====="
	make madx-macosx64-intel && ls -l madx64 && make cleantest && make tests-all ARCH=64 NOCOLOR=yes
	check_error "make tests-all for madx-macosx64-intel failed"

	echo -e "\n===== Testing madx-macosx32-intel ====="
	make madx-macosx32-intel && ls -l madx32 && make cleantest && make tests-all ARCH=32 NOCOLOR=yes
	check_error "make tests-all for madx-macosx32-intel failed"

	echo -e "\n===== Testing madx-macosx64-gnu ====="
	make madx-macosx64-gnu && ls -l madx64 && make cleantest && make tests-all ARCH=64 NOCOLOR=yes
	check_error "make tests-all for madx-macosx64-gnu failed"

	echo -e "\n===== Testing madx-macosx32-gnu ====="
	make madx-macosx32-gnu && ls -l madx32 && make cleantest && make tests-all ARCH=32 NOCOLOR=yes
	check_error "make tests-all for madx-macosx32-gnu failed"
fi

# restore the default version
make madx-macosx32 > /dev/null && make madx-macosx64 > /dev/null
check_error "unable to restore the default version"

# date & end marker
echo -e "\nFinish: `date`"
echo -e "\n===== End of build and tests ====="
