require 'mkmf'

RbConfig::MAKEFILE_CONFIG['CC'] = ENV['CC'] if ENV['CC']

$CFLAGS << " #{ENV["CFLAGS"]}"
$CFLAGS << " -g"

if RbConfig::MAKEFILE_CONFIG['CC'] =~ /gcc|clang/
  $CFLAGS << " -O3" unless $CFLAGS[/-O\d/]
  $CFLAGS << " -Wall"
end

def sys(cmd)
  puts " -- #{cmd}"
  unless ret = xsystem(cmd)
    raise "ERROR: '#{cmd}' failed"
  end
  ret
end

def preserving_globals
  values =
    $arg_config,
    $CFLAGS, $CPPFLAGS,
    $LDFLAGS, $LIBPATH, $libs
  yield
ensure
  $arg_config,
  $CFLAGS, $CPPFLAGS,
  $LDFLAGS, $LIBPATH, $libs =
    values
end

MAKE_PROGRAM = find_executable('gmake') || find_executable('make')

if MAKE_PROGRAM.nil?
  abort "ERROR: GNU make is required to build Rugged"
end

if preserving_globals { dir_config('git2') }.any?
  dir_config('git2')

  unless have_library 'git2' and have_header 'git2.h'
    abort "ERROR: Can not find libgit2 in the specified path"
  end
else
  CWD = File.expand_path(File.dirname(__FILE__))
  LIBGIT2_DIR = File.join(CWD, '..', '..', 'vendor', 'libgit2')

  if find_executable('cmake')
    Dir.chdir(LIBGIT2_DIR) do
      Dir.mkdir("build") if !Dir.exists?("build")

      Dir.chdir("build") do
        sys("cmake .. -DBUILD_CLAR=OFF -DTHREADSAFE=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_C_FLAGS=-fPIC ")
        # Grab the flags from an authoritative source
        pcfile = File.join(LIBGIT2_DIR, "build", "libgit2.pc")
        $LDFLAGS << " " + `pkg-config --libs --static #{pcfile}`.strip
        sys(MAKE_PROGRAM)
      end
    end

    dir_config('git2', "#{LIBGIT2_DIR}/include", "#{LIBGIT2_DIR}/build")
  else
    warn "WARN: CMake was not found!"
    warn "WARN: Rugged will be built without ssl or ssh support!"

    Dir.chdir(LIBGIT2_DIR) do
      sys("#{MAKE_PROGRAM} -f Makefile.embed")
    end

    dir_config('git2', "#{LIBGIT2_DIR}/include", LIBGIT2_DIR)
  end

  unless have_library 'git2' and have_header 'git2.h'
    abort "ERROR: Failed to build libgit2"
  end
end

# Compile backends

CWD ||= File.expand_path(File.dirname(__FILE__))
LIBGIT2_BACKENDS_DIR = File.join(CWD, '..', '..', 'vendor', 'libgit2-backends')

if find_executable('cmake')

  {'redis' => ['hiredis']}.each do |backend, deps|
    Dir.chdir(File.join(LIBGIT2_BACKENDS_DIR, backend)) do
      Dir.mkdir("build") if !Dir.exists?("build")
      Dir.chdir("build") do
        sys("cmake .. -DBUILD_SHARED_LIBS=OFF -DCMAKE_C_FLAGS=-fPIC -DPC_LIBGIT2_LIBRARY_DIRS=../../../libgit2/build/ -DPC_LIBGIT2_INCLUDE_DIRS=../../../libgit2/include/")
        sys(MAKE_PROGRAM)
      end
    end

    dir_config('git2-#{backend}', "#{LIBGIT2_BACKENDS_DIR}/#{backend}", "#{LIBGIT2_BACKENDS_DIR}/#{backend}/build")
    #deps.each do |d|
    #  dir_config(d)
    #end

    unless have_library "git2-#{backend}"
      abort "ERROR: Failed to build libgit2-#{backend}"
    end

    deps.each do |d|
      unless have_library(d)
        abort "ERROR: Failed to locate #{d}"
      end
    end
  end

else
  warn "WARN: CMake was not found!"
  warn "WARN: Rugged will be built without alternative backends support!"
end

unless have_library 'git2' and have_header 'git2.h'
  abort "ERROR: Failed to build libgit2"
end

create_makefile("rugged/rugged")
