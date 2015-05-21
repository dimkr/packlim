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

proc ::packlim::log {level msg} {
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

proc ::packlim::install {curl repo packages entries name trigger key} {
	# if the package is already installed, do nothing
	if {[file exists "/var/packlim/installed/$name/entry"]} {
		packlim::log warn "$name is already installed"
		return
	}

	# locate the package in the package list
	log debug "searching the package list for $name"
	try {
		set package $packages($name)
	} on error {msg opts} {
		throw error "failed to locate $name in the package list"
	}

	# install the package dependencies, recursively
	set dependencies $package(dependencies)
	if {0 < [llength $dependencies]} {
		log debug "installing the dependencies of $name"
		foreach dependency $dependencies {
			try {
				packlim::install $curl $repo $packages $entries $dependency dependency $key
			} on error {msg opts} {
				packlim::log error $msg
				packlim::cleanup
				throw error "cannot install $name"
			}
		}
	}

	# download it
	set file_name $package(file_name)
	set path "/var/packlim/downloaded/$file_name"
	file mkdir /var/packlim/downloaded
	packlim::log info "downloading $file_name"
	$curl get "$repo/$file_name" $path

	try {
		# read the package contents
		log debug "reading $file_name"

		packlim::with_file fp $path r {
			set tar [$fp read [expr [file size $path] - 68]]
			set magic [$fp read 4]
			set signature [$fp read 64]
		}

		# verify the magic number
		if {"hjkl" ne $magic} {
			throw error "$file_name is not a valid package"
		}

		# verify the package digital signature, using the public key
		if {0 == [string bytelength $key]} {
			packlim::log warn "skipping the verification of $file_name"
		} else {
			packlim::log info "verifying $file_name"
			ed25519.verify $tar $signature $key
		}

		# list the package contents
		packlim::log info "registering $name"
		set files [tar.list $tar]

		file mkdir "/var/packlim/installed/$name"

		# save the file list
		packlim::with_file fp "/var/packlim/installed/$name/files" w {
			$fp puts [join $files \n]
		}

		# save the package entry
		packlim::with_file fp "/var/packlim/installed/$name/entry" w {
			$fp puts $entries($name)
		}

		# save the package installation trigger
		packlim::with_file fp "/var/packlim/installed/$name/trigger" w {
			$fp puts $trigger
		}

		# extract the tar archive contained in the package
		packlim::log info "extracting $file_name"
		tar.extract $tar
	} on error {msg opts} {
		packlim::log error $msg
		packlim::remove_force $name
		packlim::cleanup
		throw error "failed to install $name"
	}
}

proc ::packlim::installed {} {
	set installed [dict create]

	foreach path [glob -nocomplain -directory "/var/packlim/installed" *] {
		packlim::with_file fp "$path/trigger" r {set trigger [$fp read -nonewline]}
		packlim::with_file fp "$path/entry" r {set entry [$fp read -nonewline]}
		set package [packlim::parse $entry]

		dict set installed $package(name) [list $package $trigger]
	}

	return $installed
}

# removes a package without any safety checks
proc ::packlim::remove_force {name} {
	set path "/var/packlim/installed/$name/files"
	if {[file exists $path]} {
		# read the list of files installed by the package
		packlim::with_file fp $path r {
			set files [lreverse [split [$fp read -nonewline] \n]]
		}

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
		packlim::with_file fp $path r {set entry [$fp read -nonewline]}
		set package [packlim::parse $entry]

		# delete the downloaded package
		file delete "/var/packlim/downloaded/$package(file_name)"
	}

	# remove the package installation data
	file delete -force "/var/packlim/installed/$name"
}

proc ::packlim::needed {name installed} {
	foreach installed_package [dict keys $installed] {
		set package [lindex $installed($installed_package) 0]
		if {[lsearch -bool -exact $package(dependencies) $name]} {
			return 1
		}
	}

	return 0
}

# safely removes a package
proc ::packlim::remove {name installed} {
	if {![file exists "/var/packlim/installed/$name/trigger"]} {
		packlim::log warn "$name is not installed"
		return
	}

	set trigger [lindex $installed($name) 1]
	if {"user" ne $trigger} {
		packlim::log error "$name cannot be removed; it is a $trigger package"
		return 1
	}

	if {[packlim::needed $name $installed]} {
		packlim::log error "$name cannot be removed; it is required by another package"
		return
	}

	packlim::log info "removing $name"
	packlim::remove_force $name
	packlim::cleanup
}

proc ::packlim::cleanup {} {
	log debug "removing unneeded packages"

	while {1} {
		set count 0
		set installed [packlim::installed]

		foreach name [dict keys $installed] {
			# automatically remove only dependency packages
			if {"dependency" ne [lindex $installed($name) 1]} {
				continue
			}

			if {![packlim::needed $name $installed]} {
				packlim::log info "removing $name"
				packlim::remove_force $name
				incr count
			}
		}

		if {0 == $count} {
			break
		}
	}
}

proc ::packlim::update {curl repo} {
	packlim::log info "updating the package list"
	try {
		$curl get "$repo/available" /var/packlim/available
	} on error {msg opts} {
		file delete /var/packlim/available
		throw error "failed to download the package list"
	}
}

proc ::packlim::parse {entry} {
	set fields [split $entry |]

	set package [dict create]
	dict set package name [lindex $fields 0]
	dict set package version [lindex $fields 1]
	dict set package description [lindex $fields 2]
	dict set package file_name [lindex $fields 3]
	dict set package dependencies [lindex $fields 4]

	return $package
}

proc ::packlim::with_file {fp path access script} {
	upvar 1 $fp my_fp

	try {
		open $path $access
	} on ok my_fp {
		uplevel 1 $script
	} finally {
		$my_fp close
	}
}

proc ::packlim::available {curl repo} {
	set list /var/packlim/available

	# if the package list is missing, fetch it
	if {![file exists $list]} {
		packlim::update $curl $repo
	}

	# parse the package list
	packlim::with_file fp $list r {set entries [string trimright [$fp read]]}

	set packages [dict create]
	set raw [dict create]

	foreach entry [lmap i [split $entries \n] {string trimright $i}] {
		set package [packlim::parse $entry]
		set name $package(name)
		dict set packages $name $package
		dict set raw $name $entry
	}

	list $packages $raw
}

proc ::packlim::purge {} {
	packlim::log info "purging unwanted files"
	file delete /var/packlim/available
	file delete -force /var/packlim/downloaded
}

proc ::packlim::lock {name} {
	packlim::log info "locking $name"
	packlim::with_file fp "/var/packlim/installed/$name/trigger" w {$fp puts locked}
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
