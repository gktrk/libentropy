AM_LDFLAGS = -Wl,--as-needed
bin_PROGRAMS = ssss-split
ssss_split_SOURCES = ssss.c
ssss_split_LDADD = @LIBS@

install-exec-hook:
	ln -f $(DESTDIR)$(bindir)/ssss-split$(EXEEXT) \
		$(DESTDIR)$(bindir)/ssss-combine$(EXEEXT)
