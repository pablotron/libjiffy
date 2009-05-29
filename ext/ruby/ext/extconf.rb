require 'mkmf'

if have_library('jiffy', 'jf_init')
  $LDFLAGS << ' -ljiffy'
  create_makefile("jiffy")
end

