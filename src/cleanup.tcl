proc packlim.needed {name pfix} {
	set fp [open "${pfix}var/packlim/installed/$name/reason" r]
	set reason [$fp read -nonewline]
	$fp close

	foreach path [glob -nocomplain -directory "${pfix}var/packlim/installed" *] {
		set entry [packlim.entry "$path/entry"]
		if {[lsearch -bool -exact $entry(deps) $name]} {
			return 1
		}
	}

	return 0
}

proc packlim.unneeded {pfix} {
	set unneeded {}
	set dir "${pfix}var/packlim/installed"

	foreach path [glob -nocomplain -directory $dir *] {
		set name [file tail $path]
		if {"dep" ne [packlim.reason $name $pfix]} {
			continue
		}

		if {![packlim.needed $name $pfix}] {
			lappend unneeded $name
		}
	}

	return $unneeded
}

proc packlim.cleanup {pfix} {
	$::log debug "cleaning up unneeded packages"

	while {1} {
		set unneeded [packlim.unneeded $pfix]
		set len [llength $unneeded]
		if {0 == $len} {
			break
		}

		if {1 == $len} {
			$::log debug "found 1 unneeded package"
		} else {
			$::log debug "found $len unneeded packages"
		}

		foreach name $unneeded {
			packlim.remove $name $pfix
		}
	}
}
