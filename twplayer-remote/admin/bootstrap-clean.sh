set -x
rm -frv autom4te.cache/ m4/ configure config.cache config.status config.log aclocal.m4 # libs/libltdl/
rm -fv admin/aclocal.m4 admin/compile admin/config.guess admin/config.sub admin/depcomp admin/install-sh admin/ltmain.sh admin/missing admin/mkinstalldirs
find -name Makefile.in | xargs -d'\n' rm -fv
