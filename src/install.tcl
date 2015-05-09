proc packlim.register {reason entry pkg pfix} {
	set name $entry(name)
	$::log debug "registering $name"

	set dir "${pfix}var/packlim/installed/$name"
	file mkdir $dir

	set fp [open "$dir/files" w]
	$fp puts -nonewline [join [$pkg list] \n]
	$fp close

	set fp [open "$dir/entry" w]
	$fp puts -nonewline [packlim.format $entry]
	$fp close

	set fp [open "$dir/reason" w]
	$fp puts -nonewline $reason
	$fp close
}

proc packlim.install {name pkgs repo reason key pfix} {
	if {[file exists "${pfix}var/packlim/installed/$name"]} {
		$::log warning "$name is already installed"
		return
	}

	$::log info "searching the package list for $name"
	try {
		set entry $pkgs($name)
	} on error {msg opts} {
		$::log error "failed to locate $name"
		packlim.cleanup $pfix
		throw error
	}

	try {
		foreach dep $entry(deps) {
			packlim.install $dep $pkgs $repo dep $key $pfix
		}
	} on error {msg opts} {
		packlim.cleanup $pfix
		incr opts(-level)
		return {*}$opts $msg
	}

	set file $entry(file)
	$::log info "downloading $file"
	set path "${pfix}var/packlim/downloaded/$file"
	try {
		$repo fetch $file "$path"
	} on error {msg opts} {
		$::log error "failed to download $file"
		throw error
	}

	set pkg [packlim.package $path]
	if {0 == [string bytelength $key]} {
		$::log warning "skipping the digital signature verification of $file"
	} else {
		$::log info "verifying the digital signature of $file"
		try {
			$pkg verify $key
		} on error {msg opts} {
			$::log warning "the digital signature of $file is invalid"
			throw error
		}
	}

	$::log info "installing $name"

	$::log debug "registering $name"
	packlim.register $reason $entry $pkg $pfix
	$::log debug "extracting $file"
	try {
		$pkg extract $pfix
	} on error {msg opts} {
		puts $msg $opts
		$::log error "failed to extract $file"
		packlim.unregister $name $pfix
	}
}

proc packlim.lock {name pfix} {
	set fp [open "${pfix}var/packlim/installed/$name/reason" w]
	$fp puts -nonewline lock
	$fp close
}

proc packlim.reason {name pfix} {
	set fp [open "${pfix}var/packlim/installed/$name/reason" r]
	set reason [$fp read -nonewline]
	$fp close

	return $reason
}

proc packlim.locked {name pfix} {
	if {"lock" eq [packlim.reason $name $pfix]} {
		return 1
	}

	return 0
}
