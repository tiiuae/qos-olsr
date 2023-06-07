#!/bin/sh

# The olsr.org Optimized Link-State Routing daemon (olsrd)
#
# (c) by the OLSR project
#
# See our Git repository to find out who worked on this file
# and thus is a copyright holder on it.
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in
#   the documentation and/or other materials provided with the
#   distribution.
# * Neither the name of olsr.org, olsrd nor the names of its
#   contributors may be used to endorse or promote products derived
#   from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# Visit http://www.olsr.org for more information.
#
# If you find this software useful feel free to make a donation
# to the project. For more information see the website or contact
# the copyright holders.
#

test -f ${0%/*}/src/cfgparser/oparse.c && {
  cat>&2 <<EOF
This script reformats all source files. Be careful
with doing so. You need a clean source tree, e.g.
reformatting of bison/flex output may not work well.

For these reasons: run "make uberclean" first.
EOF
  exit 1
}
test -x $PWD/${0##*/} || {
  cat>&2 <<EOF
************************************************************
Warning: about to change all files below current working dir
$PWD
************************************************************
Proceed (y/N)
EOF
  read l
  test "y" = "$l" || exit 1
}

sed -i 's/Andreas T.\{1,6\}nnesen/Andreas Tonnesen/g;s/Andreas Tønnesen/Andreas Tonnesen/g;s/Andreas TÃ¸nmnesen/Andreas Tonnesen/' $(find -type f -not -path "*/.hg*" -not -name ${0##*/})
sed -i 's///g;s/[	 ]\+$//' $(find -name "*.[ch]" -not -path "*/.hg*")

addon=
test "--cmp" = "$1" && {
  # Note: may help to compare two messy formatted source trees.
  addon="--swallow-optional-blank-lines --ignore-newlines"
  shift
}
indent $(cat ${0%/*}/src/.indent.pro) $addon $* $(find -name "*.[ch]" -not -path "*/.hg*")

rm $(find -name "*~" -not -path "*/.hg*")
