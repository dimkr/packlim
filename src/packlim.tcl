# this file is part of packlim.
#
# Copyright (c) 2015 Dima Krasner
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

proc log {level msg} {
	switch -exact $level info {
		set wrapped $msg
	} warn {
		set wrapped "$msg (!)"
	} error {
		set wrapped "$msg!"
	} debug {
		set wrapped "($msg)"
	} bug {
		set wrapped "$msg (!!!)"
	} default {
		throw error "unknown verbosity level: $level"
	}

	set now [clock format [clock seconds] -format "%d/%m %H:%M:%S"]
	puts "\[$now\]<[format %-5s $level]>: $wrapped"
}

proc install {curl repo packages entries name trigger key} {
	# if the package is already installed, do nothing
	if {[file exists "/var/packlim/installed/$name/entry"]} {
		log warn "$name is already installed"
		return
	}

	# locate the package in the package list
	log debug "searching the package list for $name"
	set package $packages($name)

	# install the package dependencies, recursively
	set dependencies $package(dependencies)
	if {0 < [llength $dependencies]} {
		log debug "installing the dependencies of $name"
		foreach dependency $dependencies {
			try {
				install $curl $repo $packages $entries $dependency dependency $key
			} on error {msg opts} {
				log error $msg
				cleanup
				throw error "cannot install $name"
			}
		}
	}

	# download it
	set file_name $package(file_name)
	set path "/var/packlim/downloaded/$file_name"
	file mkdir /var/packlim/downloaded
	log info "downloading $file_name"
	$curl get "$repo/$file_name" $path

	try {
		# read the package contents
		log debug "reading $file_name"
		set fp [open $path r]
		set tar [$fp read [expr [file size $path] - 68]]
		set magic [$fp read 4]
		set signature [$fp read 64]
		$fp close

		# verify the magic number
		if {"hjkl" ne $magic} {
			throw error "$file_name is not a valid package"
		}

		# verify the package digital signature, using the public key
		if {0 == [string bytelength $key]} {
			log warn "skipping the verification of $file_name"
		} else {
			log info "verifying $file_name"
			ed25519.verify $tar $signature $key
		}

		# list the package contents
		log info "registering $name"
		set files [tar.list $tar]

		file mkdir "/var/packlim/installed/$name"

		# save the file list
		set fp [open "/var/packlim/installed/$name/files" w]
		$fp puts [join $files \n]
		$fp close

		# save the package entry
		set fp [open "/var/packlim/installed/$name/entry" w]
		$fp puts $entries($name)
		$fp close

		# save the package installation trigger
		set fp [open "/var/packlim/installed/$name/trigger" w]
		$fp puts $trigger
		$fp close

		# extract the tar archive contained in the package
		log info "extracting $file_name"
		tar.extract $tar
	} on error {msg opts} {
		log error $msg
		remove_force $name
		cleanup
		throw error "failed to install $name"
	}
}

proc installed {} {
	set installed [dict create]

	foreach path [glob -nocomplain -directory "/var/packlim/installed" *] {
		set fp [open "$path/trigger" r]
		set trigger [$fp read -nonewline]
		$fp close

		set fp [open "$path/entry" r]
		set entry [$fp read -nonewline]
		$fp close

		set package [parse $entry]

		dict set installed $package(name) [list $package $trigger]
	}

	return $installed
}

# removes a package without any safety checks
proc remove_force {name} {
	set path "/var/packlim/installed/$name/files"
	if {[file exists $path]} {
		# read the list of files installed by the package
		set fp [open $path r]
		set files [lreverse [split [$fp read -nonewline] \n]]
		$fp close

		# delete all files, in reverse order
		foreach file $files {
			try {
				file delete $file
			} on error {msg opts} {
				if {![file isdirectory $file]} {
					throw error $msg
				}
			}
		}
	}

	# read the package entry, to obtain its file name
	set path "/var/packlim/installed/$name/entry"
	if {[file exists $path]} {
		set fp [open $path r]
		set entry [$fp read -nonewline]
		$fp close
		set package [parse $entry]

		# delete the downloaded package
		file delete "/var/packlim/downloaded/$package(file_name)"
	}

	# remove the package installation data
	file delete -force "/var/packlim/installed/$name"
}

proc needed {name installed} {
	foreach installed_package [dict keys $installed] {
		set package [lindex $installed($installed_package) 0]
		if {[lsearch -bool -exact $package(dependencies) $name]} {
			return 1
		}
	}

	return 0
}

# safely removes a package
proc remove {name installed} {
	if {![file exists "/var/packlim/installed/$name/trigger"]} {
		log warn "$name is not installed"
		return
	}

	set trigger [lindex $installed($name) 1]
	if {"user" ne $trigger} {
		log error "$name cannot be removed; it is a $trigger package"
		return 1
	}

	if {[needed $name $installed]} {
		log error "$name cannot be removed; it is required by another package"
		return
	}

	log info "removing $name"
	remove_force $name
	cleanup
}

proc cleanup {} {
	log debug "removing unneeded packages"

	while {1} {
		set count 0
		set installed [installed]

		foreach name [dict keys $installed] {
			# automatically remove only dependency packages
			if {"dependency" ne [lindex $installed($name) 1]} {
				continue
			}

			if {![needed $name $installed]} {
				log info "removing $name"
				remove_force $name
				incr count
			}
		}

		if {0 == $count} {
			break
		}
	}
}

proc update {curl repo} {
	log info "updating the package list"
	$curl get "$repo/available" /var/packlim/available
}

proc parse {entry} {
	set fields [split $entry |]

	set package [dict create]
	dict set package name [lindex $fields 0]
	dict set package version [lindex $fields 1]
	dict set package description [lindex $fields 2]
	dict set package file_name [lindex $fields 3]
	dict set package dependencies [lindex $fields 4]

	return $package
}

proc available {curl repo} {
	set list /var/packlim/available

	# if the package list is missing, fetch it
	if {![file exists $list]} {
		update $curl $repo
	}

	# parse the package list
	set fp [open $list r]
	set entries [string trimright [$fp read]]
	$fp close

	set packages [dict create]
	set raw [dict create]

	foreach entry [lmap i [split $entries \n] {string trimright $i}] {
		set package [parse $entry]
		set name $package(name)
		dict set packages $name $package
		dict set raw $name $entry
	}

	list $packages $raw
}

proc purge {} {
	log info "purging unwanted files"
	file delete /var/packlim/available
	file delete -force /var/packlim/downloaded
}

proc lock {name} {
	log info "locking $name"
	set fp [open "/var/packlim/installed/$name/trigger" w]
	$fp puts locked
	$fp close
}

proc usage {err} {
	puts stderr "Usage: [lindex $::argv 0] $err"
	exit 1
}

proc get_repo {env} {
	try {
		return $env(REPO)
	} on error {msg opts} {
		throw error "REPO is not set"
	}
}

proc main {} {
	if {2 > $::argc} {
		usage "update|available|installed|install|remove|lock|source|purge \[ARG\]..."
	}

	set env [env]
	file mkdir /var/packlim /var/packlim/installed

	# wait for other instances to terminate
	if {[lockf.locked /var/packlim/lock]} {
		log warn "another instance is running; waiting"
	}
	set lock [lockf.lock /var/packlim/lock]

	switch -exact [lindex $::argv 1] update {
		if {2 != $::argc} {
			usage update
		}

		update [curl] [get_repo $env]
	} available {
		if {2 != $::argc} {
			usage available
		}

		set packages [lindex [available [curl] [get_repo $env]] 0]
		foreach name [dict keys $packages] {
			set package $packages($name)
			puts "$package(name)|$package(version)|$package(description)"
		}
	} installed {
		if {2 != $::argc} {
			usage installed
		}

		set packages [installed]
		foreach name [dict keys $packages] {
			set package [lindex $packages($name) 0]
			puts "$package(name)|$package(version)|$package(description)"
		}
	} install {
		if {3 != $::argc} {
			usage "install NAME"
		}

		set path /etc/packlim/pub_key
		if {![file exists $path]} {
			log error "failed to read the public key"
			exit 1
		} else {
			set fp [open $path r]
			set key [$fp read -nonewline]
			$fp close
		}

		set repo [get_repo $env]
		set curl [curl]
		set available [available $curl $repo]

		install $curl $repo {*}$available [lindex $::argv 2] user $key
	} remove {
		if {3 != $::argc} {
			usage "remove NAME"
		}

		remove [lindex $::argv 2] [installed]
	} lock {
		if {3 != $::argc} {
			usage "lock NAME"
		}

		lock [lindex $::argv 2]
	} source {
		if {3 != $::argc} {
			usage "source SCRIPT"
		}

		source [lindex $::argv 2]
	} purge {
		if {2 != $::argc} {
			usage "purge"
		}

		purge
	} default {
		usage "update|available|installed|install|remove|lock|source|purge \[ARG\]..."
	}
}

proc main_wrapper {} {
	try {
		main
	} on error {msg opts} {
		if {0 < [string length $msg]} {
			log error $msg
		} else {
			log bug "caught an unknown, unhandled exception"
		}
		exit 1
	}
}
