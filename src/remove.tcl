proc packlim.unregister {name pfix} {
	$::log debug "unregistering $name"
	file delete -force "${pfix}var/packlim/installed/$name"
}

proc packlim.remove {name pfix} {
	set path "${pfix}var/packlim/installed/$name/files"

	if {![file exists $path]} {
		$::log warning "$name is not installed"
		throw error
	}

	$::log info "removing $name"

	set fp [open $path r]
	set files [lreverse [split [$fp read -nonewline] \n]]
	$fp close

	foreach file $files {
		set path "$pfix/$file"
		try {
			file delete $path
		} on error {msg opts} {
			if {![file isdirectory $path]} {
				throw error $msg
			}
		}
	}

	set entry [packlim.entry "${pfix}var/packlim/installed/$name/entry"]
	packlim.unregister $name $pfix

	file delete "${pfix}var/packlim/downloaded/$entry(file)"
}
