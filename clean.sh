#!/bin/bash -x

pushd build > /dev/null
  find mainboard -name "*.aml" -o -name "*.asl" -o -name "*.hex" -o -name "romstage*" | xargs rm -Rf
  find . -name "*.d" -o -name "*.o" -o -name "*.a" | xargs rm -Rf
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
    find . -name "*.exe" -o -name ".depend" -o -name "*.lst" -o -name "*.objdump" | xargs rm -Rf
    find . -name "*.a" -o -name "*.debug" -o -name "*.map" | xargs rm -Rf

    for directory in $( find . -name "libpayloadbin" ) ; do
        pushd $directory > /dev/null
                for file in $( find . -type f -name *.h ) ; do
                        if [ ! "$file" == "./build/libpayload-config.h" ] ; then
                                rm $file
                        fi
                done
                find . -name "lpgcc" -o -name "lpas" -o -name "lp.functions" | xargs rm -Rf
                find . -name "*.ldscript" -o -name "libpayload.config" | xargs rm -Rf
        popd > /dev/null
    done

    if [ -d ipxe ]; then
      pushd ipxe > /dev/null
      find . -name "zbin" | xargs rm -Rf
      popd > /dev/null
    fi

    if [ -d memtest86+ ]; then
      pushd memtest86+ > /dev/null
      find . -name "memtest_shared" -o -name "*.s" | xargs rm -Rf
      popd > /dev/null
    fi

    if [ -d seabios ]; then
      pushd seabios/out > /dev/null
      find . -name "*.i" -o -name "*.i.orig" -o -name "*.aml" | xargs rm -Rf
      find . -name "*.off" -o -name "*.o.tmp.c" -o -name "*.lds" | xargs rm -Rf
      find . -name "*.raw" -o -name "*.prep" -o -name "*.s" -o -name "*.hex" | xargs rm -Rf
      rm -rf include vgasrc version.c
      popd > /dev/null
      rm -rf seabios/scripts/*.pyc
    fi

    if [ -d sgabios ]; then
      pushd sgabios > /dev/null
      find . -name csum8 | xargs rm -Rf
      popd > /dev/null
    fi

    find . -type d -empty -delete

    popd > /dev/null
  fi
exit 0

