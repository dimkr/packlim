# this file is part of packlim.
#
# Copyright (c) 2015, 2016 Dima Krasner
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

package require history

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

proc ::packlim::verify {data signature key name} {
	if {[string bytelength $key] == 0} {
		packlim::log warn "skipping the verification of $name"
	} else {
		packlim::log info "verifying $name"
		ed25519.verify $data $signature $key
	}
}

proc ::packlim::build_queue {package available trigger} {
	set entries {}

	try {
		set entry $available($package)
	} on error {msg opts} {
		throw error "failed to locate $package in the package list"
	}

	foreach dependency $entry(dependencies) {
		set entries [list {*}[packlim::build_queue $dependency $available dependency] {*}$entries]
	}

	dict set entry trigger $trigger
	lappend entries $entry

	return $entries
}

proc ::packlim::install {entry path trigger key} {
	set name $entry(name)
	set file_name $entry(file_name)
	try {
		# read the package contents
		packlim::log debug "reading $file_name"

		packlim::with_file fp $path r {
			set tar [$fp read [expr [file size $path] - 68]]
			set magic [$fp read 4]
			set signature [$fp read 64]
		}

		# verify the magic number
		if {$magic ne "hjkl"} {
			throw error "$file_name is not a valid package"
		}

		# verify the package digital signature, using the public key
		packlim::verify $tar $signature $key $file_name

		# list the package contents
		packlim::log info "registering $name"
		set files [tar.list $tar]

		file mkdir "./var/packlim/installed/$name"

		# save the file list
		packlim::with_file fp "./var/packlim/installed/$name/files" w {
			$fp puts [join $files \n]
		}

		# save the raw package entry
		packlim::with_file fp "./var/packlim/installed/$name/entry" w {
			$fp puts $entry(raw)
		}

		# save the package installation trigger
		packlim::with_file fp "./var/packlim/installed/$name/trigger" w {
			$fp puts $trigger
		}

		# extract the tar archive contained in the package
		packlim::log info "extracting $file_name"
		tar.extract $tar
	} on error {msg opts} {
		packlim::log error $msg
		packlim::remove_force $name
		throw error "failed to install $name"
	}
}

proc ::packlim::fetch {repo package available trigger key} {
	packlim::log info "building the package installation queue"
	set entries [packlim::build_queue $package $available $trigger]

	set names {}
	set paths {}
	set installed {}

	# create a unique, sorted list from the package entries in the queue
	set unique_entries {}
	foreach entry [lsort -unique $entries] {
		set name $entry(name)
		if {[file exists "./var/packlim/installed/$name/entry"]} {
			packlim::log warn "$name is already installed"
			lappend installed $name
			continue
		}
		lappend paths $entry(path)
		lappend names $name
		lappend unique_entries $entry
	}

	# if all packages are installed, stop here
	set length [llength $names]
	if {$length == 0} {
		return
	}

	# download all packages
	file mkdir ./var/packlim/downloaded
	if {$length > 1} {
		packlim::log info "downloading [join [lrange $names 0 end-1] {, }] and [lindex $names end]"
	} else {
		packlim::log info "downloading $names"
	}

	curl get {*}[join [lmap entry $unique_entries {list $entry(url) $entry(path)}]]

	try {
		foreach entry $entries {
			if {[$::packlim::sigmask pending term int]} {
				throw error interrupted
			}

			set name $entry(name)

			# if the package is already installed, do nothing
			if {[lsearch $installed $name] != -1} {
				continue
			}

			# install the package
			packlim::install $entry $entry(path) $entry(trigger) $key

			# append it to the list of installed packages
			lappend installed $name
		}
	} on error {msg opts} {
		packlim::cleanup
		foreach path $paths {
			file delete $path
		}

		throw error $msg
	}
}

proc ::packlim::installed {} {
	set installed [dict create]

	foreach path [glob -nocomplain -directory "./var/packlim/installed" *] {
		packlim::with_file fp "$path/trigger" r {set trigger [$fp read -nonewline]}
		packlim::with_file fp "$path/entry" r {set entry [$fp read -nonewline]}
		set package [packlim::parse $entry]

		dict set installed $package(name) [list $package $trigger]
	}

	return $installed
}

# removes a package without any safety checks
proc ::packlim::remove_force {name} {
	set path "./var/packlim/installed/$name/files"
	if {[file exists $path]} {
		# read the list of files installed by the package
		packlim::with_file fp $path r {
			set files [lreverse [split [$fp read -nonewline] \n]]
		}

		# delete all files, in reverse order
		foreach file $files {
			try {
				file delete .$file
			} on error {msg opts} {
				if {![file isdirectory .$file]} {
					throw error $msg
				}
			}
		}
	}

	# read the package entry, to obtain its file name
	set path "./var/packlim/installed/$name/entry"
	if {[file exists $path]} {
		packlim::with_file fp $path r {set entry [$fp read -nonewline]}
		set package [packlim::parse $entry]

		# delete the downloaded package
		file delete "./var/packlim/downloaded/$package(file_name)"
	}

	# remove the package installation data
	file delete -force "./var/packlim/installed/$name"
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
	if {![file exists "./var/packlim/installed/$name/trigger"]} {
		packlim::log warn "$name is not installed"
		return
	}

	set trigger [lindex $installed($name) 1]
	if {$trigger ne "user"} {
		packlim::log error "$name cannot be removed; it is a $trigger package"
		return 1
	}

	if {[packlim::needed $name $installed]} {
		packlim::log error "$name cannot be removed; it is required by another package"
		return
	}

	packlim::log info "removing $name"
	packlim::remove_force $name
}

proc ::packlim::cleanup {} {
	packlim::log debug "removing unneeded packages"

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

proc ::packlim::update {repo} {
	packlim::log info "updating the package list"
	try {
		curl get "$repo/available" ./var/packlim/available
		if {[$::packlim::sigmask pending term int]} {
			throw error interrupted
		}
	} on error {msg opts} {
		file delete ./var/packlim/available
		throw error "failed to download the package list"
	}

	try {
		curl get "$repo/available.sig" ./var/packlim/available.sig
	} on error {msg opts} {
		file delete ./var/packlim/available ./var/packlim/available.sig
		throw error "failed to download the package list digital signature"
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

	set my_fp [open $path $access]
	try {
		uplevel 1 $script
	} finally {
		$my_fp close
	}
}

proc ::packlim::available {repo key} {
	set list ./var/packlim/available
	set sig ./var/packlim/available.sig

	# if the package list is missing, fetch it
	if {![file exists $list] || ![file exists $sig]} {
		packlim::update $repo
	}

	# parse the package list
	packlim::with_file fp $list r {set data [$fp read]}
	packlim::with_file fp $sig r {
		set signature [$fp read]
	}
	packlim::verify $data $signature $key "the package list"

	set packages [dict create]
	set raw [dict create]

	foreach entry [lmap i [split [string trimright $data] \n] {string trimright $i}] {
		set package [packlim::parse $entry]
		dict set package raw $entry
		set file_name $package(file_name)
		dict set package url "$repo/$file_name"
		dict set package path "./var/packlim/downloaded/$file_name"
		dict set packages $package(name) $package
	}

	return $packages
}

proc ::packlim::purge {} {
	packlim::log info "purging unwanted files"
	file delete ./var/packlim/available.sig ./var/packlim/available
	file delete -force ./var/packlim/downloaded
}

proc ::packlim::lock {name} {
	packlim::log info "locking $name"
	packlim::with_file fp "./var/packlim/installed/$name/trigger" w {$fp puts locked}
}

proc ::packlim::package {priv pub} {
	set tar [stdin read]
	set sig [ed25519.sign $tar $priv $pub]
	puts -nonewline "${tar}hjkl${sig}"
}

proc ::packlim::shell {} {
	packlim::log info "packlim interactive shell, version [info version]"
	packlim::log info "Available commands: [join [info commands packlim::*] {, }]"

	while {1} {
		set cmd ""
		set prompt "# "

		while {1} {
			if {[history getline $prompt chunk] < 0} {
				exit 0
			}
			if {$cmd ne ""} {
				append cmd \n
			}

			append cmd $chunk
			if {[info complete $cmd]} {
				break
			}

			set prompt ". "
		}

		if {[string length $cmd] > 1} {
			history add $cmd
		}

		try {
			set output [eval $cmd]
		} on {error return break continue signal} {output opts} {
			if {$output ne ""} {
				puts "> [info returncodes $opts(-code)]:"
			} else {
				puts "> [info returncodes $opts(-code)]"
			}
		}

		foreach line [split $output \n] {
			puts ". $line"
		}
	}
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
