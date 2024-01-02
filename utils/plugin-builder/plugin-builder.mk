
# Case conversion macros. This is inspired by the 'up' macro from gmsl
# (http://gmsl.sf.net). It is optimised very heavily because these macros
# are used a lot. It is about 5 times faster than forking a shell and tr.
#
# The caseconvert-helper creates a definition of the case conversion macro.
# After expansion by the outer $(eval ), the UPPERCASE macro is defined as:
# $(strip $(eval __tmp := $(1))  $(eval __tmp := $(subst a,A,$(__tmp))) ... )
# In other words, every letter is substituted one by one.
#
# The caseconvert-helper allows us to create this definition out of the
# [FROM] and [TO] lists, so we don't need to write down every substition
# manually. The uses of $ and $$ quoting are chosen in order to do as
# much expansion as possible up-front.
#
# Note that it would be possible to conceive a slightly more optimal
# implementation that avoids the use of __tmp, but that would be even
# more unreadable and is not worth the effort.

[FROM] := a b c d e f g h i j k l m n o p q r s t u v w x y z - .
[TO]   := A B C D E F G H I J K L M N O P Q R S T U V W X Y Z _ _

define caseconvert-helper
$(1) = $$(strip \
	$$(eval __tmp := $$(1))\
	$(foreach c, $(2),\
		$$(eval __tmp := $$(subst $(word 1,$(subst :, ,$c)),$(word 2,$(subst :, ,$c)),$$(__tmp))))\
	$$(__tmp))
endef

$(eval $(call caseconvert-helper,UPPERCASE,$(join $(addsuffix :,$([FROM])),$([TO]))))
$(eval $(call caseconvert-helper,LOWERCASE,$(join $(addsuffix :,$([TO])),$([FROM]))))

# utilities
blank =
comma = ,
space = $(blank) $(blank)

# Sanitize macro cleans up generic strings so it can be used as a filename
# and in rules. Particularly useful for VCS version strings, that can contain
# slashes, colons (OK in filenames but not in rules), and spaces.
sanitize = $(subst $(space),_,$(subst :,_,$(subst /,_,$(strip $(1)))))

# github(user,package,version): returns site of GitHub repository
github = https://github.com/$(1)/$(2)/archive/$(3)

# custom for PawPaw

PKG = $(call UPPERCASE,$(call sanitize,$(pkgname)))
$(PKG)_PKGDIR = $(CURDIR)/src/mod-plugin-builder/plugins/package/$(pkgname)

RENAME_EXTENSION = $(foreach f,$(2)$(3)$(4)$(5)$(6)$(7)$(8)$(9)$(10)$(11)$(12)$(13)$(14)$(15)$(16)$(17)$(18)$(19),mv $(f) $(subst .so,.$(1),$(f));) echo

BR2_PAWPAW = y
BR2_SKIP_LTO = y
BR2_TARGET_OPTIMIZATION =

MAKE1 = make -j1

ifeq ($(MACOS),true)
PARALLEL_JOBS = $(shell sysctl -n hw.logicalcpu)
else
PARALLEL_JOBS = $(shell nproc)
endif

ifeq ($(WINDOWS),true)
HOST_DIR = $(PAWPAW_PREFIX)-host
else
HOST_DIR = $(PAWPAW_PREFIX)
endif

TARGET_CFLAGS = $(CFLAGS)
TARGET_CXXFLAGS = $(CXXFLAGS)
TARGET_LDFLAGS = $(LDFLAGS)

TARGET_DIR = $(PAWPAW_PREFIX)

TARGET_MAKE_ENV = env
TARGET_MAKE_ENV += CFLAGS="$(TARGET_CFLAGS)"
TARGET_MAKE_ENV += CXXFLAGS="$(TARGET_CXXFLAGS)"
TARGET_MAKE_ENV += LDFLAGS="$(TARGET_LDFLAGS)"

ifneq ($(MACOS)$(WINDOWS),true)
TARGET_MAKE_ENV += LD_LIBRARY_PATH="$(PAWPAW_PREFIX)/usr/lib"
endif

ifeq ($(MACOS),true)
STRIP = true
endif

ifneq ($(TOOLCHAIN_PREFIX),)
BR2_EXTRA_CONFIGURE_OPTS  = --host=$(TOOLCHAIN_PREFIX)
BR2_EXTRA_CONFIGURE_OPTS += ac_cv_host=$(TOOLCHAIN_PREFIX)
BR2_EXTRA_CONFIGURE_OPTS += ac_cv_build=$(shell uname -m)-linux-gnu
endif

define generic-package

endef

define autotools-package


define $$(PKG)_CONFIGURE_CMDS
	(cd $$($$(PKG)_BUILDDIR) && \
	./configure \
		--disable-debug \
		--disable-doc \
		--disable-docs \
		--disable-maintainer-mode \
		--prefix='/usr' \
		$(BR2_EXTRA_CONFIGURE_OPTS) \
		$$($$(PKG)_CONF_OPTS) \
	)
endef

define $$(PKG)_BUILD_CMDS
	$(MAKE) -C $$($$(PKG)_BUILDDIR)
endef

ifndef $$(PKG)_INSTALL_TARGET_CMDS
define $$(PKG)_INSTALL_TARGET_CMDS
	$(MAKE) -C $$($$(PKG)_BUILDDIR) install DESTDIR=$(PAWPAW_PREFIX)
endef
endif

endef

define cmake-package

define $(PKG)_CONFIGURE_CMDS
	rm -f $$($$(PKG)_BUILDDIR)/CMakeCache.txt && \
	$$(CMAKE) -S $$($$(PKG)_BUILDDIR) -B $$($$(PKG)_BUILDDIR) \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_LIBDIR=lib \
		-DCMAKE_INSTALL_PREFIX='/usr' \
		--no-warn-unused-cli \
		$$($$(PKG)_CONF_OPTS)
endef

define $(PKG)_BUILD_CMDS
	$(MAKE) -C $$($$(PKG)_BUILDDIR)
endef

ifndef $(PKG)_INSTALL_TARGET_CMDS
define $(PKG)_INSTALL_TARGET_CMDS
	$(MAKE) -C $$($$(PKG)_BUILDDIR) install DESTDIR=$(PAWPAW_PREFIX)
endef
endif

endef

include $(CURDIR)/src/mod-plugin-builder/plugins/package/$(pkgname)/$(pkgname).mk

TARGET_CFLAGS += $($(PKG)_CFLAGS)
TARGET_CXXFLAGS += $($(PKG)_CXXFLAGS)

$(PKG)_DLVERSION = $(call sanitize,$(strip $($(PKG)_VERSION)))
$(PKG)_BUILDDIR = $(PAWPAW_BUILDDIR)/$(pkgname)-$($(PKG)_DLVERSION)

ifdef $(PKG)_SOURCE
$(PKG)_DLSITE = $($(PKG)_SITE)/$($(PKG)_SOURCE)
else
$(PKG)_SOURCE = $(pkgname)-$($(PKG)_DLVERSION).tar.gz
$(PKG)_DLSITE = $($(PKG)_SITE).tar.gz
endif

$(PKG)_DLFILE = $(PAWPAW_DOWNLOADDIR)/$($(PKG)_SOURCE)

STAMP_EXTRACTED  = $($(PKG)_BUILDDIR)/.stamp_extracted
STAMP_PATCHED    = $($(PKG)_BUILDDIR)/.stamp_patched
STAMP_CONFIGURED = $($(PKG)_BUILDDIR)/.stamp_configured
STAMP_BUILT      = $($(PKG)_BUILDDIR)/.stamp_built
STAMP_INSTALLED  = $($(PKG)_BUILDDIR)/.stamp_installed
STAMP_PINSTALLED = $($(PKG)_BUILDDIR)/.stamp_post_installed

PAWPAW_TMPDIR = /tmp/PawPaw
PAWPAW_TMPNAME = git-dl

all: $(STAMP_PINSTALLED)

$(STAMP_PINSTALLED): $(STAMP_INSTALLED)
	$(call $($(PKG)_POST_INSTALL_TARGET_HOOKS))
ifeq ($(MACOS),true)
	$(foreach b,$($(PKG)_BUNDLES),sed -i 's|lv2:binary <\([_a-zA-Z0-9-]*\)\.so>|lv2:binary <\1\.dylib>|g' $(PAWPAW_PREFIX)/usr/lib/lv2/$(b)/manifest.ttl;)
	$(foreach b,$($(PKG)_BUNDLES),$(call RENAME_EXTENSION,dylib,$(wildcard $(PAWPAW_PREFIX)/usr/lib/lv2/$(b)/*.so));)
else ifeq ($(WINDOWS),true)
	$(foreach b,$($(PKG)_BUNDLES),sed -i 's|lv2:binary <\([_a-zA-Z0-9-]*\)\.so>|lv2:binary <\1\.dll>|g' $(PAWPAW_PREFIX)/usr/lib/lv2/$(b)/manifest.ttl;)
	$(foreach b,$($(PKG)_BUNDLES),rm -f $(PAWPAW_PREFIX)/usr/lib/lv2/$(b)/*.dll.a;)
	$(foreach b,$($(PKG)_BUNDLES),$(call RENAME_EXTENSION,dll,$(wildcard $(PAWPAW_PREFIX)/usr/lib/lv2/$(b)/*.so));)
endif
	touch $@

$(STAMP_INSTALLED): $(STAMP_BUILT)
	$($(PKG)_INSTALL_TARGET_CMDS)
	touch $@

$(STAMP_BUILT): $(STAMP_CONFIGURED)
ifeq ($(MACOS),true)
	$(foreach p,$(wildcard $($(PKG)_BUILDDIR)/Makefile $($(PKG)_BUILDDIR)/*/makefile),\
		sed -i -e 's/-Wl,--gc-sections//g' $(p);)
	$(foreach p,$(wildcard $($(PKG)_BUILDDIR)/Makefile),\
		sed -i -e 's/-Wl,--as-needed//g' $(p);)
	$(foreach p,$(wildcard $($(PKG)_BUILDDIR)/*/makefile),\
		sed -i -e 's/-Wl,--no-undefined//g' $(p);)
	$(foreach p,$(wildcard $($(PKG)_BUILDDIR)/*/makefile),\
		sed -i -e 's/-Wl,--exclude-libs,ALL//g' $(p);)
	$(foreach p,$(wildcard $($(PKG)_BUILDDIR)/*/makefile),\
		sed -i -e 's/-Wl,-z,relro,-z,now//g' $(p);)
	$(foreach p,$(wildcard $($(PKG)_BUILDDIR)/*/makefile),\
		sed -i -e 's/-Wl,-z,noexecstack//g' $(p);)
endif
	$($(PKG)_BUILD_CMDS)
	touch $@

$(STAMP_CONFIGURED): $(STAMP_PATCHED)
ifeq ($($(PKG)_AUTORECONF),YES)
	(cd $($(PKG)_BUILDDIR) && \
		aclocal --force && \
		libtoolize --force --automake --copy && \
		autoheader --force && \
		autoconf --force && \
		automake -a --copy \
	)
endif
	$($(PKG)_CONFIGURE_CMDS)
	touch $@

$(STAMP_PATCHED): $(STAMP_EXTRACTED)
	$(foreach p,$(sort $(wildcard $($(PKG)_PKGDIR)/*.patch)),patch -p1 -d '$($(PKG)_BUILDDIR)' -i $(p);)
	touch $@

$(STAMP_EXTRACTED): $($(PKG)_DLFILE)
	mkdir -p '$($(PKG)_BUILDDIR)'
	tar -xf '$($(PKG)_DLFILE)' -C '$($(PKG)_BUILDDIR)' --strip-components=1
	touch $@

$($(PKG)_DLFILE):
ifeq ($($(PKG)_SITE_METHOD),git)
	rm -rf '$(PAWPAW_TMPDIR)'
	git clone '$($(PKG)_SITE)' '$(PAWPAW_TMPDIR)/$(PAWPAW_TMPNAME)' && \
	git -C '$(PAWPAW_TMPDIR)/$(PAWPAW_TMPNAME)' checkout '$($(PKG)_VERSION)' && \
	touch '$(PAWPAW_TMPDIR)/$(PAWPAW_TMPNAME)/.gitmodules' && \
	sed -i 's|git://github.com/|https://github.com/|g' '$(PAWPAW_TMPDIR)/$(PAWPAW_TMPNAME)/.gitmodules' && \
	git -C '$(PAWPAW_TMPDIR)/$(PAWPAW_TMPNAME)' submodule update --recursive --init && \
	tar --exclude='.git' -czf '$($(PKG)_DLFILE)' -C '$(PAWPAW_TMPDIR)' '$(PAWPAW_TMPNAME)'
	rm -rf '$(PAWPAW_TMPDIR)'
else
	curl -Lo '$($(PKG)_DLFILE)' '$($(PKG)_DLSITE)'
endif
