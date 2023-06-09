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

# avoid the $(if) everywhere
C=$(if $(CFGDIR),$(CFGDIR)/)

# add the variables as we may have others already there
SRCS += $(foreach file,olsrd_conf olsrd_conf_checksum oparse oscan cfgfile_gen,$(C)$(file).c)
OBJS += $(foreach file,olsrd_conf olsrd_conf_checksum oparse oscan cfgfile_gen,$(C)$(file).o)
HDRS += $(foreach file,olsrd_conf olsrd_conf_checksum oparse,$(C)$(file).h)

$(C)oscan.c: $(C)oscan.lex $(C)Makefile
ifeq ($(VERBOSE),0)
	@echo "[FLEX] $@"
endif
	$(MAKECMDPREFIX)$(FLEX) -Cem -o"$@-tmp" "$<"
	$(MAKECMDPREFIX)sed	-e '/^static/s/yy_get_next_buffer[\(][\)]/yy_get_next_buffer(void)/' \
		-e '/^static/s/yy_get_previous_state[\(][\)]/yy_get_previous_state(void)/' \
		-e '/^static/s/yygrowstack[\(][\)]/yygrowstack(void)/' \
		-e '/^static/s/input[\(][\)]/input(void)/' \
		-e '/^static  *void  *yy_fatal_error/s/^\(.*)\);$$/\1 __attribute__((noreturn));/' \
		-e 's/register //' \
		-e '/^#line/s/$(call quote,$@-tmp)/$(call quote,$@)/' \
		< "$@-tmp" >"$@"
	$(MAKECMDPREFIX)$(RM) "$@-tmp"

# we need a dependency to generate oparse before we compile oscan.c
$(C)oscan.o: $(C)oparse.c
$(C)oscan.o: CFLAGS := $(filter-out -Wunreachable-code -Wsign-compare,$(CFLAGS)) -Wno-sign-compare -Wno-switch-default
# we need potentially another -I directory
$(C)oscan.o: CPPFLAGS += $(if $(CFGDIR),-I$(CFGDIR)) -DYY_NO_INPUT

$(C)oparse.c: $(C)oparse.y $(C)olsrd_conf.h $(C)Makefile
ifeq ($(VERBOSE),0)
	@echo "[BISON] $@"
endif
	$(MAKECMDPREFIX)$(BISON) -d -o "$@" "$<"
	$(MAKECMDPREFIX)sed -e 's/register //' "$@" > "$@.o" && mv "$@.o" "$@"


$(C)oparse.o: CFLAGS := $(filter-out -Wunreachable-code,$(CFLAGS))

# and a few files to be cleaned
TMPFILES += $(foreach pat,oscan.c oparse.c oparse.h,$(C)$(pat) $(C)$(pat)-tmp)
