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

package require packlim

proc get_key {{type pub}} {
	set path .@CONF_DIR@/packlim/${type}_key

	if {![file exists $path]} {
		packlim::log error "failed to read a digital signing key: $path"
		exit 1
	}

	packlim::with_file fp $path r {set key [$fp read -nonewline]}
	return $key
}

proc main {} {
	if {$::argc < 2} {
		usage "update|available|installed|install|remove|autoremove|lock|source|shell|purge|package \[ARG\]..."
	}

	# if no keys are present, generate a new pair
	if {![file exists .@CONF_DIR@/packlim/pub_key] && ![file exists .@CONF_DIR@/packlim/priv_key]} {
		set keys [ed25519.keypair]

		try {
			packlim::with_file fp .@CONF_DIR@/packlim/priv_key w {$fp puts -nonewline [lindex $keys 0]}
		} on error {msg opts} {
			file delete .@CONF_DIR@/packlim/priv_key
			throw error $msg
		}

		try {
			packlim::with_file fp .@CONF_DIR@/packlim/pub_key w {$fp puts -nonewline [lindex $keys 1]}
		} on error {msg opts} {
			file delete .@CONF_DIR@/packlim/priv_key
			file delete .@CONF_DIR@/packlim/pub_key
			throw error $msg
		}
	}

	set env [env]
	set ::packlim::sigmask [sigmask term int]
	file mkdir ./var/packlim ./var/packlim/installed

	# wait for other instances to terminate
	if {[lockf.locked ./var/packlim/lock]} {
		packlim::log warn "another instance is running; waiting"
	}
	set lock [lockf.lock ./var/packlim/lock]

	switch -exact [lindex $::argv 1] update {
		if {$::argc != 2} {
			usage update
		}

		packlim::update [get_repo $env]
	} available {
		if {$::argc != 2} {
			usage available
		}

		set key [get_key]
		set installed [packlim::installed]
		set available [packlim::available [get_repo $env] $key]
		foreach name [dict keys $available] {
			if {[dict exists $installed $name]} {
				continue
			}
			set package $available($name)
			puts "$package(name)|$package(version)|$package(description)"
		}
	} installed {
		if {$::argc != 2} {
			usage installed
		}

		set packages [packlim::installed]
		foreach name [dict keys $packages] {
			set package [lindex $packages($name) 0]
			if {"dependency" ne [lindex $packages($name) 1]} {
				puts "$package(name)|$package(version)|$package(description)"
			}
		}
	} install {
		if {$::argc < 3} {
			usage "install NAME..."
		}

		set repo [get_repo $env]
		set key [get_key]
		set available [packlim::available $repo $key]

		foreach package [lrange $::argv 2 end] {
			packlim::fetch $repo $package $available user $key
		}
	} remove {
		if {$::argc < 3} {
			usage "remove NAME..."
		}

		foreach package [lrange $::argv 2 end] {
			packlim::remove $package [packlim::installed]
		}
	} autoremove {
		if {$::argc > 2} {
			foreach package [lrange $::argv 2 end] {
				packlim::remove $package [packlim::installed]
			}
		}

		packlim::cleanup
	} lock {
		if {$::argc != 3} {
			usage "lock NAME"
		}

		packlim::lock [lindex $::argv 2]
	} source {
		if {$::argc != 3} {
			usage "source SCRIPT"
		}

		source [lindex $::argv 2]
	} purge {
		if {$::argc != 2} {
			usage "purge"
		}

		packlim::purge
	} package {
		if {$::argc != 2} {
			usage "package"
		}

		set priv [get_key priv]
		set pub [get_key]

		packlim::package $priv $pub
	} shell {
		if {$::argc != 2} {
			usage "shell"
		}

		packlim::shell
	} default {
		usage "update|available|installed|install|remove|autoremove|lock|source|shell|purge|package \[ARG\]..."
	}
}

try {
	main
} on error {msg opts} {
	if {[string length $msg] > 0} {
		packlim::log error $msg
	} else {
		packlim::log bug "caught an unknown, unhandled exception"
	}
	exit 1
}
