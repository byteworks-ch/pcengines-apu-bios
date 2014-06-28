#!/bin/bash -x

pushd build > /dev/null
  find mainboard -name "*.aml" -o -name "*.asl" -o -name "*.hex" -o -name "romstage*" | xargs rm -Rf
  find . -name "*.d" -o -name "*.o" | xargs rm -Rf
  find . -type d -empty -delete
  rm -rf mrc.cache build.h ./*.bin ./*pre* ldoptions option_table.h
  rm -rf cbfstool rmodtool cpu cbfs
  rm -rf generated coreboot*.rom soc util/nvramtool/cli util/nvramtool/accessors
  rm -f image.img
popd > /dev/null

  if [ -d payloads ]; then
    pushd payloads > /dev/null
    find . -name "*.d" -o -name "*.o" -o -name "*.elf" -o -name "*.rom" | xargs rm -Rf
    find . -name "*.bin" -o -name "*.tmp" -o -name "*.zbin" -o -name "*.zinfo" | xargs rm -Rf

    if [ -d memtest86+ ]; then
      rm -rf memtest86+/memtest_shared *.s
    fi

    if [ -d seabios ]; then
      pushd seabios/out > /dev/null
      rm -rf include src vgasrc *.o.tmp.c *.lds version.c *.objdump
      rm -rf *.debug *.raw *.prep
      popd > /dev/null
      rm -rf seabios/scripts/*.pyc
    fi
    popd > /dev/null
  fi
exit 0

